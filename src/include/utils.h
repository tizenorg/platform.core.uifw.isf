/*
 * Copyright (c) 2012 - 2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#define CHECK_ARRAY_INDEX(index, array_size) (index >= 0 && index < array_size)
#define DEGREE_TO_SCLROTATION(degree) \
    (degree == 90 ? ROTATION_90_CW : \
    (degree == 180 ? ROTATION_180 : \
    (degree == 270 ? ROTATION_90_CCW : \
    ROTATION_0)))
#define ROTATION_TO_DEGREE(rotation) \
    (rotation == ROTATION_90_CW ? 90 : \
    (rotation == ROTATION_180 ? 180 : \
    (rotation == ROTATION_90_CCW ? 270 : \
    0)))

#endif
