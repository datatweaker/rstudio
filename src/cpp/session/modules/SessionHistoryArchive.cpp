/*
 * SessionHistoryArchive.cpp
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#include "SessionHistoryArchive.hpp"

#include <string>

#include <core/Error.hpp>
#include <core/Log.hpp>
#include <core/FilePath.hpp>
#include <core/DateTime.hpp>
#include <core/FileSerializer.hpp>

#include <r/session/RConsoleHistory.hpp>

#include <session/SessionModuleContext.hpp>

using namespace core;

namespace session {
namespace modules { 
namespace history {

namespace {

FilePath historyDatabaseFilePath()
{
   return module_context::userScratchPath().complete("history_database");
}

void writeEntry(double timestamp, const std::string& command, std::ostream* pOS)
{
   *pOS << std::fixed << std::setprecision(0)
        << timestamp << ":" << command;
}

std::string migratedHistoryEntry(const std::string& command)
{
   std::ostringstream ostr ;
   writeEntry(0, command, &ostr);
   return ostr.str();
}

void attemptRhistoryMigration()
{
   Error error = writeCollectionToFile<r::session::ConsoleHistory>(
                                                historyDatabaseFilePath(),
                                                r::session::consoleHistory(),
                                                migratedHistoryEntry);

   // log any error which occurs
   if (error)
      LOG_ERROR(error);
}

// simple reader for parsing lines of history file
class HistoryEntryReader
{
public:
   HistoryEntryReader() : nextIndex_(0) {}

   ReadCollectionAction operator()(const std::string& line,
                                   HistoryEntry* pEntry)
   {
      // if the line doesn't have a ':' then ignore it
      if (line.find(':') == std::string::npos)
         return ReadCollectionIgnoreLine;

      pEntry->index = nextIndex_++;
      std::istringstream istr(line);
      istr >> pEntry->timestamp ;
      istr.ignore(1, ':');
      std::getline(istr, pEntry->command);

      // if we had a read failure log it and return ignore state
      if (!istr.fail())
      {
         return ReadCollectionAddLine;
      }
      else
      {
         LOG_ERROR_MESSAGE("unexpected io error reading history line: " +
                           line);
         return ReadCollectionIgnoreLine;
      }
   }
private:
   int nextIndex_;
};


} // anonymous namespace

HistoryArchive& historyArchive()
{
   static HistoryArchive instance;
   return instance;
}

Error HistoryArchive::add(const std::string& command)
{
   // reset the cache (since this write will invalidate the current one,
   // no sense in keeping our cache around in memory)
   entries_.clear();
   entryCacheLastWriteTime_ = -1;

   // write the entry to the file
   std::ostringstream ostrEntry ;
   double currentTime = core::date_time::millisecondsSinceEpoch();
   writeEntry(currentTime, command, &ostrEntry);
   ostrEntry << std::endl;
   return appendToFile(historyDatabaseFilePath(), ostrEntry.str());
}

const std::vector<HistoryEntry>& HistoryArchive::entries() const
{
   // calculate path to history db
   FilePath historyDBPath = historyDatabaseFilePath();

   // if the file doesn't exist then clear the collection
   if (!historyDBPath.exists())
   {
      entries_.clear();
   }

   // otherwise check for divergent lastWriteTime and read the file
   // if our internal list isn't up to date
   else if (historyDBPath.lastWriteTime() != entryCacheLastWriteTime_)
   {
      entries_.clear();
      Error error = readCollectionFromFile<std::vector<HistoryEntry> >(
                                                   historyDBPath,
                                                   &entries_,
                                                   HistoryEntryReader());
      if (error)
      {
         LOG_ERROR(error);
      }
      else
      {
         entryCacheLastWriteTime_ = historyDBPath.lastWriteTime();
      }

   }

   // return entries
   return entries_;
}

void HistoryArchive::migrateRhistoryIfNecessary()
{
   // if the history database doesn't exist see if we can migrate the
   // old .Rhistory file
   FilePath historyDBPath = historyDatabaseFilePath();
   if (!historyDBPath.exists())
      attemptRhistoryMigration() ;
}


} // namespace history
} // namespace modules
} // namesapce session

