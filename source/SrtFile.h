// ----------------------------------------------------------------------------
// SrtFile.h
// Simple SRT file library, by Louis de Carufel.
//
// Can be used to parse an SRT file, renumber the subtitles,
// offset timecodes forwards or backwards, and back write into
// an SRT file.
//
// The streams can be std::fstream or std::stringstream. 
//
// Usage example:
//
//  using namespace std;
//  fstream fileStream("inputfile.srt", std::ios_base::in);
//  SrtFile srtFile(fileStream);
//  fileStream.close();
//
//  srtFile.Renumber(1);
//  srtFile.OffsetInMilliseconds(-2000);
//
//  fstream outputStream("outputfile.srt", ios_base::out | ios_base::trunc);
//  srtFile.WriteToFile(outputStream);
//  outputStream.close();
// ----------------------------------------------------------------------------

#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <stdarg.h>
#include <algorithm>

namespace SrtFileInternal
{
    bool ReadLine(std::istream& stream, std::string& str)
    {
        char buffer[1024];
        if (stream.getline(buffer, sizeof(buffer)))
        {
            str = buffer;
            return true;
        }
        return false;
    }

    void WriteLine(std::ostream& stream, const char* format, ...)
    {
        char buffer[1024];
        va_list argptr;
        va_start(argptr, format);
        int len = vsnprintf(buffer, sizeof(buffer), format, argptr);
        va_end(argptr);

        if (len < 1)
            return;

        stream.write(buffer, strnlen(buffer, sizeof(buffer)));

        if (buffer[len - 1] != '\n')
            stream << '\n';
    }

    bool IsBlankLine(const std::string& str)
    {
        return str.find_first_not_of(" \t\n\v\f\r") == std::string::npos;
    }
}

class SrtTimeCode
{
public:
    SrtTimeCode() = default;
    SrtTimeCode(const char* str)
    {
        SetFromString(str);
    }
    SrtTimeCode(int hours, int minutes, int seconds, int milliseconds)
    {
        Set(hours, minutes, seconds, milliseconds);
    }

    void Clear()
    {
        *this = {};
    }

    bool IsValid() const
    {
        return m_timeCodeMs >= 0L;
    }

    bool operator<(const SrtTimeCode& other) const
    {
        return m_timeCodeMs < other.m_timeCodeMs;
    }

    void Set(long hours, long minutes, long seconds, long milliseconds)
    {
        m_timeCodeMs = milliseconds;
        m_timeCodeMs += seconds * 1000;
        m_timeCodeMs += minutes * 60 * 1000;
        m_timeCodeMs += hours * 60 * 60 * 1000;
        m_timeCodeMs = std::max(m_timeCodeMs, 0L);
    }

    void SetMilliseconds(long milliseconds)
    {
        m_timeCodeMs = milliseconds;
    }

    bool SetFromString(const std::string& str)
    {
        int hours, minutes, seconds, milliseconds;
        int nbValues = sscanf_s(str.c_str(), "%02d:%02d:%02d,%03d", &hours, &minutes, &seconds, &milliseconds);
        if (nbValues == 4)
            Set(hours, minutes, seconds, milliseconds);
        return nbValues == 4;
    }

    void Get(int& hours, int& minutes, int& seconds, int& milliseconds) const
    {
        long remainder = std::max(m_timeCodeMs, 0L);
        hours = remainder / (60 * 60 * 1000);
        remainder -= hours * 60 * 60 * 1000;
        minutes = remainder / (60 * 1000);
        remainder -= minutes * 60 * 1000;
        seconds = remainder / 1000;
        milliseconds = remainder - seconds * 1000;
    }

    long GetMilliseconds() const
    {
        return m_timeCodeMs;
    }

    void WriteToString(std::string& str) const
    {
        int hours, minutes, seconds, milliseconds;
        Get(hours, minutes, seconds, milliseconds);
        char textBuffer[16];
        std::snprintf(textBuffer, sizeof(textBuffer), "%02d:%02d:%02d,%03d", hours, minutes, seconds, milliseconds);
        str.assign(textBuffer);
    }

    void OffsetInMilliseconds(long offset)
    {
        m_timeCodeMs = std::max(m_timeCodeMs + offset, 0L);
    }

private:
    long m_timeCodeMs = -1;
};

class SrtSubtitle
{
public:
    SrtSubtitle() = default;
    SrtSubtitle(std::iostream& stream)
    {
        ReadFromFile(stream);
    }

    void Clear()
    {
        *this = {};
    }

    bool IsValid() const
    {
        return m_index >= 0L && m_startTime.IsValid() && m_endTime.IsValid() && !m_textLines.empty();
    }

    bool ReadFromFile(std::istream& stream)
    {
        if (!stream.good())
            return false;

        std::string indexLine;
        while (SrtFileInternal::ReadLine(stream, indexLine))
        {
            if (SrtFileInternal::IsBlankLine(indexLine))
            {
                m_extra.append("\n");
                continue;
            }

            // Try to read subtitle index
            if (indexLine.find_first_of("0123456789") == std::string::npos ||   // No numbers on line
                indexLine.find("-->") != std::string::npos ||                   // Misplaced timecode arrow
                sscanf_s(indexLine.c_str(), "%d", &m_index) == 0)               // Unreadable number
            {
                m_extra.append(indexLine);
                m_extra.append("\n");
                continue;
            }
            
            // Try to read subtitle timing
            std::string timeCodeLine;
            if (!SrtFileInternal::ReadLine(stream, timeCodeLine))
                continue;

            size_t arrowPos = timeCodeLine.find("-->");
            if (arrowPos == std::string::npos ||                                // No arrow on line
                !m_startTime.SetFromString(timeCodeLine) ||                     // Unreadable start time
                !m_endTime.SetFromString(timeCodeLine.substr(arrowPos + 3)))    // Unreadable end time
            {
                m_extra.append(indexLine);
                m_extra.append("\n");
                m_extra.append(timeCodeLine);
                m_extra.append("\n");
                continue;
            }
                
            if (m_endTime < m_startTime)
            {
                m_endTime.SetMilliseconds(m_startTime.GetMilliseconds() + 1);
            }

            // Read optional coordinates
            size_t lastCommaPos = timeCodeLine.find_last_of(',');
            if (lastCommaPos != std::string::npos && lastCommaPos + 5 < timeCodeLine.size())
            {
                m_coordinates = timeCodeLine.substr(lastCommaPos + 5);
            }

            // Read subtitle text lines
            std::string textLine;
            while (SrtFileInternal::ReadLine(stream, textLine))
            {
                if (SrtFileInternal::IsBlankLine(textLine))
                    break;
                m_textLines.push_back(textLine);
            }

            break;
        }

        return IsValid();
    }

    void WriteToFile(std::ostream& stream, bool ignoreExtra) const
    {
        if (!IsValid() || !stream.good())
            return;

        if (!ignoreExtra)
        {
            SrtFileInternal::WriteLine(stream, m_extra.c_str());
        }

        SrtFileInternal::WriteLine(stream, "%d", m_index);
        
        std::string startTimeCodeStr, endTimeCodeStr;
        m_startTime.WriteToString(startTimeCodeStr);
        m_endTime.WriteToString(endTimeCodeStr);
        SrtFileInternal::WriteLine(stream, "%s --> %s%s%s", startTimeCodeStr.c_str(), endTimeCodeStr.c_str(), m_coordinates.empty() ? "" : " ", m_coordinates.c_str());

        for (const std::string& textLine : m_textLines) 
        {
            SrtFileInternal::WriteLine(stream, "%s", textLine.c_str());
        }
    }

    void OffsetInMilliseconds(long offset)
    {
        m_startTime.OffsetInMilliseconds(offset);
        m_endTime.OffsetInMilliseconds(offset);
    }

    void SetIndex(long index)
    {
        m_index = index;
    }

public:
    long m_index = -1;
    SrtTimeCode m_startTime;
    SrtTimeCode m_endTime;
    std::string m_coordinates;
    std::vector<std::string> m_textLines;
    std::string m_extra;
};

class SrtFile
{
public:
    SrtFile() = default;
    SrtFile(std::istream& stream)
    {
        ReadFromFile(stream);
    }

    void Clear()
    {
        *this = {};
    }

    bool IsValid() const
    {
        return !m_subtitles.empty();
    }

    // Reads an SRT file from the given stream.
    // Can be a std::fstream or a std::stringstream. 
    bool ReadFromFile(std::istream& stream)
    {
        if (!stream.good())
            return false;

        SrtSubtitle subtitle;
        while (subtitle.ReadFromFile(stream))
        {
            m_subtitles.emplace_back(std::move(subtitle));
            subtitle.Clear();
        }

		if (!subtitle.IsValid() && !subtitle.m_extra.empty())
			m_extra = subtitle.m_extra;

        return IsValid();
    }

    // Writes SRT file contents to the given stream.
    // Can be a std::fstream or a std::stringstream. 
    void WriteToFile(std::ostream& stream, bool ignoreExtra = false) const
    {
        if (!IsValid() || !stream.good())
            return;

		for (int subIndex = 0; subIndex < m_subtitles.size(); ++subIndex)
        {
			const SrtSubtitle& subtitle = m_subtitles[subIndex];
            subtitle.WriteToFile(stream, ignoreExtra);

			if (subIndex < m_subtitles.size() - 1)
				SrtFileInternal::WriteLine(stream, "\n");
        }

        if (!ignoreExtra && !m_extra.empty())
        {
            SrtFileInternal::WriteLine(stream, "\n");
            SrtFileInternal::WriteLine(stream, m_extra.c_str());
        }
    }

    // Offset the timecode of all subtitle by the given number of milliseconds.
    // A positive offset will make the subtitles appear later.
    // A negative offset will make the subtitles appear sooner. 
    void OffsetInMilliseconds(long offset)
    {
        if (offset < 0 && !m_subtitles.empty())
        {
            const long firstSubStartTime = m_subtitles[0].m_startTime.GetMilliseconds();
            offset = -std::min(firstSubStartTime, -offset);
        }

        for (SrtSubtitle& subtitle : m_subtitles)
        {
            subtitle.OffsetInMilliseconds(offset);
        }
    }

    // Renumbers all subtitles sequentially, starting at the specified index.
    // Returns the index of the last subtitle.
    long Renumber(long startIndex = 1L)
    {
        startIndex = std::max(startIndex, 1L);
        for (SrtSubtitle& subtitle : m_subtitles)
        {
            subtitle.SetIndex(startIndex++);
        }
        return startIndex - 1;
    }

public:
    std::vector<SrtSubtitle> m_subtitles;
    std::string m_extra;
};
