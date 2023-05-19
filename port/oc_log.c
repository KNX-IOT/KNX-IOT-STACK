/*
// Copyright (c) 2022-2023 Cascoda Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#include <stdarg.h>
#include <stdio.h>

#define OUTPUT_FILE_NAME "stack_print_output.txt"
static FILE *fptr = NULL;

void
oc_file_print(char *format, ...)
{
  va_list args;
  if (fptr == NULL) {
    fptr = fopen(OUTPUT_FILE_NAME, "w");
  }
  if (fptr) {
    va_start(args, format);
    vfprintf(fptr, format, args);
    va_end(args);
    // flush the file
    fflush(fptr);
  }
}