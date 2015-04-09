/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2013 Samsung Electronics Co., Ltd.
 *
 * Contact: Wonkeun Oh <wonkeun.oh@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef __MOTION_INPUT_H__
#define __MOTION_INPUT_H__

#include "MotionDataType.h"
#include <iostream>
#include <vector>
#include <deque>

using namespace std;

class Motion_Input
{
public :
    Point2D _Cursor_delta;      // Mouse delta X,Y in screen

private:
    deque<Point3Df>  _GyrFilterBuffer;
    deque<Point3Df>  _AccFilterBuffer;

    double _Cursor_acc_val;
    double _Cursor_gain_val;  //16:9
    int _Cursor_smooth_val;
    double _Cursor_speed_val;
    int _uinput_fd;
    int _filterNumber;

public:

    Motion_Input();
    ~Motion_Input();

    void init_motion_input();

    bool calculate_cursor_delta_value( Point3Df AccData, Point3Df GyrData, Point2D *Cursor_delta);

    bool filter_raw_sensor_data(Point3Df *origin_AccData, Point3Df *origin_GyrData);

    bool Set_cursor_movement_acceleration(double Cursor_acc_val);

    bool Set_cursor_movement_gain(double Cursor_gain_val);

    bool Set_cursor_movement_smoothness(int Cursor_smooth_val);

    bool Set_cursor_movement_speed(double Cursor_speed_val);

    bool Set_filter_number(int filter_number);

    //calculate history mean gyroscope value
    double cal_gyr_mean();

    bool copy_sensor_data(Point3Df *source_Data, Point3Df *Data);

};
