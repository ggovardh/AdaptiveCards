#include "Utils.h"
#include <time.h>
#include <windows.h>

namespace RendererQml
{
    int Utils::HexStrToInt(const std::string& str)
    {
        std::string::size_type sz;
        const int i = std::stoi(str, &sz, 16);
        if (sz != str.size())
        {
            throw std::invalid_argument("Number is not a valid hexadecimal");
        }
        return i;
    }

    int Utils::GetSpacing(const AdaptiveCards::SpacingConfig& spacingConfig, const AdaptiveCards::Spacing spacing)
    {
        switch (spacing)
        {
        case AdaptiveCards::Spacing::None:
            return 0;
        case AdaptiveCards::Spacing::Small:
            return spacingConfig.smallSpacing;
        case AdaptiveCards::Spacing::Medium:
            return spacingConfig.mediumSpacing;
        case AdaptiveCards::Spacing::Large:
            return spacingConfig.largeSpacing;
        case AdaptiveCards::Spacing::ExtraLarge:
            return spacingConfig.extraLargeSpacing;
        case AdaptiveCards::Spacing::Padding:
            return spacingConfig.paddingSpacing;
        default:
            return spacingConfig.defaultSpacing;
        }
    }

    const AdaptiveCards::ContainerStyleDefinition& Utils::GetContainerStyle(const AdaptiveCards::ContainerStylesDefinition& containerStyles, AdaptiveCards::ContainerStyle style)
    {
        switch (style)
        {
        case AdaptiveCards::ContainerStyle::Accent:
            return containerStyles.accentPalette;
        case AdaptiveCards::ContainerStyle::Warning:
            return containerStyles.warningPalette;
        case AdaptiveCards::ContainerStyle::Attention:
            return containerStyles.attentionPalette;
        case AdaptiveCards::ContainerStyle::Good:
            return containerStyles.goodPalette;
        case AdaptiveCards::ContainerStyle::Emphasis:
            return containerStyles.emphasisPalette;
        case AdaptiveCards::ContainerStyle::Default:
        default:
            return containerStyles.defaultPalette;
        }
    }

    bool Utils::CaseInsensitiveCompare(const std::string& str1, const std::string& str2)
    {
        return ((str1.size() == str2.size()) && std::equal(str1.begin(), str1.end(), str2.begin(), [](const char& c1, const char& c2) {
            return (c1 == c2 || std::tolower(c1) == std::tolower(c2));
            }));
    }

    bool Utils::IsNullOrWhitespace(const std::string& str)
    {
        return std::find_if(str.begin(), str.end(), [](unsigned char ch) { return !std::isspace(ch); }) == str.end();
    }

    std::string& Utils::RightTrim(std::string& str)
    {
        str.erase(std::find_if(str.rbegin(), str.rend(), [](int c) { return !std::isspace(c); }).base(), str.end());
        return str;
    }

    std::string& Utils::LeftTrim(std::string& str)
    {
        str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int c) { return !std::isspace(c); }));
        return str;
    }

    std::string& Utils::Trim(std::string& str)
    {
        return RightTrim(LeftTrim(str));
    }

    std::string& Utils::Replace(std::string& str, char what, char with)
    {
        if (!with)
        {
            str.erase(std::remove(str.begin(), str.end(), what), str.end());
        }
        else
        {
            std::replace(str.begin(), str.end(), what, with);
        }
        return str;
    }

    std::string& Utils::Replace(std::string& str, const std::string& what, const std::string& with)
    {
        if (!what.empty())
        {
            size_t start = 0;
            while ((start = str.find(what, start)) != std::string::npos)
            {
                str.replace(start, what.length(), with);
                start += with.length();
            }
        }
        return str;
    }

    std::string& Utils::ToLower(std::string& str)
    {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }

    bool Utils::TryParse(const std::string& str, double& value)
    {
        try
        {
            std::string::size_type sz;
            value = std::stod(str, &sz);
            return sz == str.size();
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool Utils::EndsWith(const std::string& str, const std::string& end)
    {
        if (end.size() > str.size())
        {
            return false;
        }
        return str.compare(str.length() - end.length(), end.length(), end) == 0;
    }

    std::string TextUtils::ApplyTextFunctions(const std::string& text, const std::string& lang)
    {
        std::smatch oneMatch;
        std::string result = text;
        std::string::const_iterator searchLoc(text.cbegin());
        while (std::regex_search(searchLoc, text.cend(), oneMatch, m_textFunctionRegex))
        {
            if (oneMatch[1] == "DATE" || oneMatch[1] == "TIME")
            {
                std::tm utcTm = {};
                std::stringstream tmss(oneMatch[2]);
                tmss >> std::get_time(&utcTm, "%Y-%m-%dT%H:%M:%S");

                if (!tmss.fail())
                {
                    std::tm lt = {};
                    if (GetLocalTime(oneMatch[3], utcTm, lt))
                    {
                        std::string format = "%x";
                        if (oneMatch[1] == "DATE")
                        {
                            if (oneMatch[4] == "LONG")
                            {
                                format = "%A, %B %d, %Y"; //There is no equivalent for C# "D" format in C++
                            }
                            else if (oneMatch[4] == "SHORT")
                            {
                                format = "%a, %b %d, %Y";
                            }
                            else
                            {
                                format = "%x";
                            }
                        }
                        else if (oneMatch[1] == "TIME")
                        {
                            if (oneMatch[4] != "")
                            {
                                searchLoc = oneMatch.suffix().first;
                                continue;
                            }
                            format = "%I:%M %p";
                        }

                        std::stringstream ss2;
                        ss2.imbue(TextUtils::GetValidCultureInfo(lang));
                        ss2 << std::put_time(&lt, format.c_str());

                        result = Utils::Replace(result, oneMatch[0], ss2.str());
                    }
                }
            }
            searchLoc = oneMatch.suffix().first;
        }
        return result;
    }

    std::locale TextUtils::GetValidCultureInfo(const std::string& lang)
    {
        return std::locale("en_US");
        // TODO: shared object model does not support wstring so non-english languages may not render correctly.

        //if (lang.empty())
        //{
        //    return std::locale();
        //}
        //try
        //{
        //    return std::locale(lang);
        //}
        //catch (const std::runtime_error&)
        //{
        //    return std::locale();
        //}
    }

    bool TextUtils::GetLocalTime(const std::string& tzOffset, std::tm& utcTm, std::tm& lt)
    {
        lt = {};
        std::stringstream tzss(tzOffset);
        char offsetType;
        tzss >> offsetType;

        time_t tzt = 0;
        if (offsetType != 'Z' && !tzss.fail())
        {
            std::tm tzm = {};
            tzss >> std::get_time(&tzm, "%H:%M");
            if (!tzss.fail())
            {
                tzt = (tzm.tm_hour * 60 * 60) + (tzm.tm_min * 60) + tzm.tm_sec;
                if (offsetType == '-')
                {
                    tzt = -tzt;
                }
            }
        }

        if (tzss.fail())
        {
            return false;
        }

#ifdef _WIN32
        const time_t utct = _mkgmtime(&utcTm) - tzt;
        localtime_s(&lt, &utct);
#else
        const time_t utct = timegm(&utcTm) - tzt;
        localtime_r(&utct, &lt);
#endif

        return true;
    }

    std::regex TextUtils::m_textFunctionRegex(R"xxx(\{\{(DATE|TIME)\(([\d]{4}-[\d]{2}-[\d]{2}T[\d]{2}:[\d]{2}:[\d]{2})(Z|(?:(?:-|\+)\d{2}:\d{2}))(?:,\s*(SHORT|LONG|COMPACT)\s*)??\)\}\})xxx");

	std::string Utils::GetHorizontalAlignment(std::string aligntype)
	{
		if (aligntype.compare("center") == 0)
			return "Text.AlignHCenter";
		else if (aligntype.compare("right") == 0)
			return "Text.AlignRight";
		else
			return "Text.AlignLeft";

	}

	std::string Utils::GetWeight(AdaptiveCards::TextWeight weight)
	{
		switch (weight)
		{
			case AdaptiveCards::TextWeight::Lighter:
				return "Font.ExtraLight";
			case AdaptiveCards::TextWeight::Bolder:
				return "Font.Bold";
			default:
				return "Font.Normal";
		}
	}

	std::string Utils::GetDate(std::string date, bool MiniumDate_MaximumDate)
	{
		//Input format:"yyyy-mm-dd" , Output Format:"mm-dd-yyyy" or "new Date(yyyy,mm,dd)"
		std::vector<std::string> date_split = Utils::splitString(date, '-');
		if (MiniumDate_MaximumDate == true)
		{
			return "new Date(" + date_split[0] + "," + date_split[1] + "," + date_split[2] + ")";
		}
		return date_split[1] + "-" + date_split[2] + "-" + date_split[0];
	}

	bool Utils::isValidTime(std::string& time)
	{
		//24 hour format check
		try
		{
			std::vector<std::string> time_split = Utils::splitString(time, ':');
			if (stoi(time_split[0]) >= 0 && stoi(time_split[0]) <= 23)
			{
				if(stoi(time_split[1]) >= 0 && stoi(time_split[1]) <= 59)
				return true;
			}
			return false;
		}
		catch(...)
		{
			return false;
		}
	}

	std::string Utils::defaultTimeto12hour(std::string& defaultTime)
	{
		std::vector<std::string> time_split=Utils::splitString(defaultTime,':');

		std::string tt = "AM";

		if (stoi(time_split[0]) > 12)
		{
			tt = "PM";
		}

		time_split[0] = std::to_string(stoi(time_split[0]) > 12 ? stoi(time_split[0]) - 12 : stoi(time_split[0]));

		return Formatter() << std::setfill('0') << std::setw(2) << time_split[0] << ":" << time_split[1] << " " << tt;
	}

    std::string Utils::ConvertToLowerIdValue(const std::string& value)
    {
        std::string newId = value;

        transform(newId.begin(), newId.end(), newId.begin(), ::tolower);
        return newId;
    }

	bool Utils::isSystemTime12Hour()
	{
		char dateTimeBuffer[80];
		struct tm newtime;
		time_t now = time(0);
		localtime_s(&newtime, &now);
		setlocale(LC_TIME, "");
		strftime(dateTimeBuffer, 80, "%c", &newtime);

		std::vector<std::string> time_split = Utils::splitString( std::string(dateTimeBuffer), ' ');

		if (time_split.size() == 2)
		{
			return false;
		}
		return true;
	}

	std::vector<std::string> Utils::splitString(const std::string& string, char delimiter)
	{
		std::vector<std::string> splitElements;
		std::stringstream ss(string);
		while (ss.good())
		{
			std::string substr;
			getline(ss, substr, delimiter);
			splitElements.push_back(substr);
		}
		return splitElements;
	}

}
