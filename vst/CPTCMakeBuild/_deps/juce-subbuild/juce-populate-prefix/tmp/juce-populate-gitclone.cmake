# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if(EXISTS "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/juce-populate-gitclone-lastrun.txt" AND EXISTS "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/juce-populate-gitinfo.txt" AND
  "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/juce-populate-gitclone-lastrun.txt" IS_NEWER_THAN "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/juce-populate-gitinfo.txt")
  message(STATUS
    "Avoiding repeated git clone, stamp file is up to date: "
    "'C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/juce-populate-gitclone-lastrun.txt'"
  )
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: 'C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-src'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "C:/Program Files/Git/cmd/git.exe"
            clone --no-checkout --config "advice.detachedHead=false" "https://github.com/juce-framework/JUCE.git" "juce-src"
    WORKING_DIRECTORY "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps"
    RESULT_VARIABLE error_code
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/juce-framework/JUCE.git'")
endif()

execute_process(
  COMMAND "C:/Program Files/Git/cmd/git.exe"
          checkout "8.0.8" --
  WORKING_DIRECTORY "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-src"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: '8.0.8'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "C:/Program Files/Git/cmd/git.exe" 
            submodule update --recursive --init 
    WORKING_DIRECTORY "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-src"
    RESULT_VARIABLE error_code
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: 'C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-src'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/juce-populate-gitinfo.txt" "C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/juce-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: 'C:/Users/antho/Documents/projets-ia/ai-dj/vst/CPTCMakeBuild/_deps/juce-subbuild/juce-populate-prefix/src/juce-populate-stamp/juce-populate-gitclone-lastrun.txt'")
endif()
