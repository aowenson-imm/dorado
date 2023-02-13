include(GetPrerequisites)

macro(RESOLVE_SYMLINKS INPUT_LIST OUTPUT_LIST)
    foreach(LIB IN ITEMS ${INPUT_LIST})
        # This goes through the input list and sets up a new output list, where
        # output list contains all the original libraries, plus the resolution
        # of any symlinks.
        # This is mildly non-trivial because there may be a chain of symlinks,
        # e.g. libzmq.so -> libzmq.so.3 -> libzmq.so.3.1.0, and we want them
        # all.
        # In addition, we'll inspect each file and see if its SONAME matches the
        # filename, and if it doesn't we'll try and find the SONAME
        # library in the same directory.

        # The basic script was bagsied from here:
        # https://stackoverflow.com/a/29708367/6103219
        # And then modified to check for further dependencies

        # Start by storing the lib itself
        list(APPEND ${OUTPUT_LIST} "${LIB}")
        # Now go through all the symlinks, one at a time
        while(IS_SYMLINK ${LIB})
          # Grab path to directory containing the current symlink.
          get_filename_component(sym_path "${LIB}" DIRECTORY)

          #Resolve one level of symlink, store resolved path back in lib.
          execute_process(COMMAND readlink "${LIB}"
            RESULT_VARIABLE errMsg
            OUTPUT_VARIABLE LIB
            OUTPUT_STRIP_TRAILING_WHITESPACE)

          # Check to make sure readlink executed correctly.
          if(errMsg AND (NOT "${errMsg}" EQUAL "0"))
            message(FATAL_ERROR "Error calling readlink on library ${LIB}")
          endif()

          # Convert resolved path to an absolute path, if it isn't one already.
          if(NOT IS_ABSOLUTE "${LIB}")
            set(LIB "${sym_path}/${LIB}")
          endif()

          # Append resolved path to symlink resolution list.
          list(APPEND ${OUTPUT_LIST} "${LIB}")
        endwhile()
        # We've now got a lib that isn't a symlink.
        # Check and see if it's a text file -- there are some Linux
        # libs (libpthread, libm) that are configured this way. We'll just
        # ignore them.
        if(UNIX)
            find_program(FILE file)
            execute_process(
                COMMAND ${FILE} "${LIB}"
                OUTPUT_VARIABLE FILE_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(${FILE_OUTPUT} MATCHES "ASCII")
                message(STATUS "Shared library ${LIB} is a text file -- not adding to redist_libs target")
                continue()
            endif()
        endif()
        # Check and see if there are other dependencies that lib needs that
        # are in the same folder.
        set(PREREQS "")
        get_filename_component(LIB_DIR "${LIB}" DIRECTORY)
        GET_PREREQUISITES("${LIB}" PREREQS 1 0 . "${LIB_DIR}")
        foreach(PREREQ IN ITEMS ${PREREQS})
            # Replace rpath entries where possible
            string(REPLACE "@rpath" "${LIB_DIR}" PREREQ "${PREREQ}")
            list(APPEND ${OUTPUT_LIST} "${PREREQ}")
        endforeach()
        if(APPLE)
            # TODO: otool-based extraction of SONAME
            # At the moment this isn't needed as the OSX archives build fine,
            # but it might be later if we add additional libraries.
        else()
            find_program(READELF readelf)
            execute_process(
                COMMAND ${READELF} -d "${LIB}"
                OUTPUT_VARIABLE READELF_OUTPUT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            # This regex looks for:
            # SONAME  The string SONAME
            # [^[]+   One or more characters that are not an open square bracket
            # \\[     One open square bracket
            # [^]]+   One or more characters that are not a closing square bracket,
            #
            # We're trying to pull out library names from patterns that look like this:
            #
            # 0x000000000000000e (SONAME)             Library soname: [libzmq.so.1]
            string(REGEX MATCH
                "SONAME[^[]+\\[[^]]+"
                MATCH_OUTPUT
                "${READELF_OUTPUT}"
            )
            # Now that we have the general line we're after, throw away everything except
            # for the library name using REGEX REPLACE
            # See above for how the regex is structured
            string(REGEX REPLACE
                "SONAME[^[]+\\[([^]]+)"
                "\\1"
                SONAME_LIB
                "${MATCH_OUTPUT}"
            )
            list(APPEND ${OUTPUT_LIST} "${LIB_DIR}/${SONAME_LIB}")
        endif()
    endforeach()
endmacro(RESOLVE_SYMLINKS)