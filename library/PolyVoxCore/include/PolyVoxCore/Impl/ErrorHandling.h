/*******************************************************************************
Copyright (c) 2005-2009 David Williams

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution. 	
*******************************************************************************/

#ifndef __PolyVox_ErrorHandling_H__
#define __PolyVox_ErrorHandling_H__

#include <cstdlib>  //For std::exit
#include <iostream> //For std::cerr

#define POLYVOX_ASSERTS_ENABLED

#if defined(_MSC_VER)
    #define POLYVOX_HALT() __debugbreak()
#else
    #define POLYVOX_HALT() std::exit(EXIT_FAILURE)
#endif

#define POLYVOX_UNUSED(x) do { (void)sizeof(x); } while(0)

/*
 * Assertions
 * ----------
 * The code below implements a custom assert function called POLYVOX_ASSERT which has a number of advantages compared
 * to the standard C/C++ assert(). It is inspired by http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/ 
 * which provides code under the MIT license.
 */

#ifdef POLYVOX_ASSERTS_ENABLED

	#define POLYVOX_ASSERT(condition, message) \
		do \
		{ \
			if (!(condition)) \
			{ \
				std::cerr << std::endl << std::endl; \
				std::cerr << "    PolyVox Assertion Failed!" << std::endl; \
				std::cerr << "    -------------------------" << std::endl; \
				std::cerr << "    Condition: " << #condition << std::endl; \
				std::cerr << "    Message:   " << (message) << std::endl; \
				std::cerr << "    Location:  " << "Line " << __LINE__ << " of " << __FILE__ << std::endl << std::endl; \
				POLYVOX_HALT(); \
			} \
		} while(0)

#else
	
	#define POLYVOX_ASSERT(condition, message) \
		do { POLYVOX_UNUSED(condition); POLYVOX_UNUSED(message); } while(0)
	
#endif

#endif //__PolyVox_ErrorHandling_H__
