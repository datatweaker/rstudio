/*
 * SpellingEngine.hpp
 *
 * Copyright (C) 2009-11 by RStudio, Inc.
 *
 * This program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifndef CORE_SPELLING_SPELLING_ENGINE_HPP
#define CORE_SPELLING_SPELLING_ENGINE_HPP

#include <string>
#include <vector>

#include <boost/utility.hpp>

namespace core {

class Error;

namespace spelling {

class SpellingEngine : boost::noncopyable
{
public:
   virtual ~SpellingEngine() {}
   virtual Error checkSpelling(const std::string& langId,
                               const std::string& word,
                               bool *pCorrect) = 0;

   virtual Error suggestionList(const std::string& langId,
                                const std::string& word,
                                std::vector<std::string>* pSugs) = 0;

   virtual Error addWord(const std::string& langId,
                         const std::string& word,
                         bool *pAdded) = 0;

   virtual Error removeWord(const std::string& langId,
                            const std::string& word,
                            bool *pRemoved) = 0;
};

} // namespace spelling
} // namespace core 


#endif // CORE_SPELLING_SPELLING_ENGINE_HPP

