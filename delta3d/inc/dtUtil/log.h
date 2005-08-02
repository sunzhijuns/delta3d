/*
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2005, BMH Associates, Inc.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * @author Matthew W. Campbell
*/
#ifndef __Log__h
#define __Log__h

#include <string>
#include <map>
#include <fstream>

#include <osg/Referenced>
#include <osg/ref_ptr>
#include <dtCore/export.h>

namespace dtUtil 
{

    /**
     * Helps making logging a little easier.  However, printf style
    *   logging is desired, you cannot use this macro.
     */
     #define LOG_DEBUG(msg)\
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_DEBUG);

     #define LOG_INFO(msg)\
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_INFO);

     #define LOG_WARNING(msg)\
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_WARNING);

     #define LOG_ERROR(msg)\
        dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_ERROR);

     #define LOGN_DEBUG(name, msg)\
        dtUtil::Log::GetInstance(name).LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_DEBUG);

     #define LOGN_INFO(name, msg)\
        dtUtil::Log::GetInstance(name).LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_INFO);

     #define LOGN_WARNING(name, msg)\
        dtUtil::Log::GetInstance(name).LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_WARNING);

     #define LOGN_ERROR(name, msg)\
        dtUtil::Log::GetInstance(name).LogMessage(__FUNCTION__, __LINE__, msg,dtUtil::Log::LOG_ERROR);

    /**
     * Log class which the engine uses for all of its logging
     * needs.  The log file is formatted using html tags,
     * therefore, any browser should display the log without
     *  any problems.
     */
    class DT_EXPORT Log : public osg::Referenced 
    {
    public:

        /**
         * The different types of log messages.
         */
        enum LogMessageType
        {
            LOG_DEBUG,
            LOG_INFO,
            LOG_WARNING,
            LOG_ERROR
        };


        /**
         * Logs a time-stamped message.
         * @param source - String identifier of the source of the message.
         *  (__FUNCTION__ is useful here.
         * @param line the line number.
         *  @param msg - Message to display.
         *  @param msgType - Type of message being displayed. (error,warning,info)
         */
        void LogMessage(const std::string &source, int line, const std::string &msg,
                            LogMessageType msgType = LOG_INFO);

        /**
         * Little more sophisticated method for logging messages.  Allows for
         *  an unlimited number of parameters in a C-style printf syntax.
         *  @param msgType - Type of message being displayed. (error,warning,info)
         *  @param source - String identifier of the source of the message.
         *  @param line - the line number.
         *  @param msg - Printf - style format string.
         *  @note
         *      Max length of the string to be printed is 2048 characters.
         */
        void LogMessage(LogMessageType msgType, const std::string &source, int line,
                            const char *msg, ...);

        /**
         * Little more sophisticated method for logging messages.  Allows for
         *  an unlimited number of parameters in a C-style printf syntax.
         *  @param msgType - Type of message being displayed. (error,warning,info)
         *  @param source - String identifier of the source of the message.
         *  @param msg - Printf - style format string.
         *  @note
         *      Max length of the string to be printed is 2048 characters.
         */
        void LogMessage(LogMessageType msgType, const std::string &source,
                            const char *msg, ...);
        /**
         * Inserts a horizontal rule into the log file.
         */
        void LogHorizRule();

        /**
         * @return true if log messages of the given level will be sent to the log
         * output
         * @param msgType the type of message to query about.
         */
        bool IsLevelEnabled(LogMessageType msgType) const { return msgType >= mLevel; };

        /**
         * Sets the lowest level of logging that will be logged.
         * If the level in set to Debug, all messages will be sent. If the level is set error, only
         * errors will be sent.
         * @param msgType the new logging level
         */
        void SetLogLevel(LogMessageType msgType)  { mLevel = msgType; };

        std::string GetLogLevelString( LogMessageType msgType);

        /*
         * Retrieve singleton instance of the log class.
         */
        static Log& GetInstance();

        /*
         * Retrieve singleton instance of the log class for a give string name.
         * @param name logger name
         */
        static Log& GetInstance(const std::string& name);


    //Constructor and destructor are both protected since this is a singleton.
    protected:
        /**
         * Opens the log file and writes the html header information.
         */
        Log();

        /**
         * Writes any closing html tags and closes the log file.
         */
        ~Log();

    private:
        static const std::string mDefaultName;

        LogMessageType mLevel;


    };
}

#endif

