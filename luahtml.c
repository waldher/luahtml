/******************************************************************************
* Copyright (C) 2010 Benjamin Waldher.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

#include "luahtml.h"

void luaHtml_call(lua_State *L, char *fileName){
	int state = 0; // 0 = No State  1 = Inside a scriptlet  2 = Inside a print scriptlet
	int charState = 0; // 0 = No State  1 = <  2 = <%  3 = <%=  4 = %  5 = %>
	int line = 1;
	int error = 0;

	int sizeAlloced = INITIAL_SIZE;
	int length = 0;
	char *result = malloc(INITIAL_SIZE); //Grow this using geometric progression
	
	FILE *fp;

	fp = fopen(fileName, "r");

	if(!fp){
		return;
	}

	appendToResult(&result, "io.write(\"", &length, &sizeAlloced);

	while(!feof(fp)){
		char c = (char)fgetc(fp);

		if(feof(fp)){
			break;
		}

		if(c == '\n'){
			line++;
		}
	
		if(c == '<') {
			charState = 1;
		} else if(c == '%') {
			if(charState == 1){
				if(state == 1 || state == 2){
					printf("LuaHtml Error: Scriptlet started while already inside of a scriptlet at line: %d", line);
					error = 1;
					charState = 0;
				} else if(state == 0) {
					charState = 2;
				}
			} else {
				charState = 4;
			}
		} else if(c == '>' && charState == 4){
			if(charState == 4){
				if(state == 0){
					printf("LuaHtml Error: Scriptlet ended while not inside of a scriptlet at line: %d", line);
					error = 1;
					charState = 0;
				} else {
					if(state == 1){
						appendToResult(&result, "; io.write(\"", &length, &sizeAlloced);
					} else if(state == 2){
						appendToResult(&result, "); io.write(\"", &length, &sizeAlloced);
					}
					state = 0;
				}
			} else {
				charState = 0;
			}
		} else if(c == '=' && charState == 2){
			if(state == 1 || state == 2){
				printf("LuaHtml Error: Scriptlet started while already inside of a scriptlet at line: %d", line);
				error = 1;
				charState = 0;
			} else if(state == 0) {
				charState = 0;
				state = 2;
				appendToResult(&result, "\"); io.write(", &length, &sizeAlloced);
			}
		} else {
			if(charState == 2){
				charState = 0;
				state = 1;
				appendToResult(&result, "\"); ", &length, &sizeAlloced);
			} else if(charState == 1){
				charState = 0;
				appendCharToResult(&result, '<', &length, &sizeAlloced);
			}
			if(state == 0 && (c == '"' || c == '\\')){
				appendCharToResult(&result, '\\', &length, &sizeAlloced);
			}
			if(state == 0 && c == '\n'){
				appendToResult(&result, "\\n\");", &length, &sizeAlloced);
				error = luaL_loadbuffer(L, result, length, "line") || lua_pcall(L, 0, 0, 0);

				if(error){
					fprintf(stderr, "%s", lua_tostring(L, -1));
					lua_pop(L, 1);
				}

				length = 0;
				appendToResult(&result, "io.write(\"", &length, &sizeAlloced);
			} else {
				appendCharToResult(&result, c, &length, &sizeAlloced);
			}
		}
	}
	appendToResult(&result, "\");", &length, &sizeAlloced);
	error = luaL_loadbuffer(L, result, length, "line") || lua_pcall(L, 0, 0, 0);

	if(error){
		fprintf(stderr, "%s", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	free(result);
}

void appendToResult(char **result, char *toAppend, int *length, int *sizeAlloced){
	char * charPtr = (*result)+(*length);
	int pos = 0;

	while(toAppend[pos] != 0){
		if((*length) >= (*sizeAlloced)-1){
			*sizeAlloced *= 2;
			*result = realloc(*result, *sizeAlloced);
			charPtr = (*result)+(*length);
		}
		*charPtr = toAppend[pos];
		charPtr++;
		pos++;
		(*length)++;
	}
	*charPtr = 0;
}

void appendCharToResult(char **result, char toAppend, int *length, int *sizeAlloced){
	char * charPtr = (*result)+(*length);
	int pos = 0;

	if((*length) >= (*sizeAlloced)-1){
		*sizeAlloced *= 2;
		*result = realloc(*result, *sizeAlloced);
		charPtr = (*result)+(*length);
	}
	*charPtr = toAppend;
	charPtr++;
	pos++;
	(*length)++;
	*charPtr = 0;
}
