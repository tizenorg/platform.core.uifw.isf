#include "motion_input.h"
#include <opencv2/opencv.hpp>

template<typename _Tp>
void hpfitler(const _Tp* Y, _Tp* Trend, int numObs, double smoothing)
{
    if (numObs < 2)
    {
        return;
    }

    CvMat* A=cvCreateMat(numObs,numObs,CV_32FC1);
    cvZero(A);
    CvMat* b= cvCreateMat(numObs,1,CV_32FC1);
    for (int i=0;i<numObs;i++)
    {
        cvSetReal1D(b,i,Y[i]);
    }
    if (numObs==3)
    {
        for (int i=0;i<numObs;i++)
        {
            cvSetReal2D(A,i,i,smoothing);
        }
        CvMat* A_mul= cvCreateMat(3,3,CV_32FC1);
        float A_mul_float[]={ 1, -2, 1, -2, 4, -2, 1, -2, 1};
        cvSetData(A_mul,A_mul_float,CV_AUTOSTEP);
        cvMatMul(A,A_mul,A);
        cvReleaseMat(&A_mul);
    }
    else
    {
        float e[]={smoothing,-4*smoothing,(1+6*smoothing),-4*smoothing,smoothing};
        for (int i=0;i<numObs;i++)
        {
            cvSetReal2D(A,i,i,e[2]);
        }
        for (int i=0;i<numObs-1;i++)
        {
            cvSetReal2D(A,i,i+1,e[1]);
            cvSetReal2D(A,i+1,i,e[1]);
        }
        for (int i=0;i<numObs-2;i++)
        {
            cvSetReal2D(A,i,i+2,e[0]);
            cvSetReal2D(A,i+2,i,e[0]);
        }
        cvSetReal2D(A,0,0,1+smoothing);
        cvSetReal2D(A,0,1,-2*smoothing);
        cvSetReal2D(A,1,0,-2*smoothing);
        cvSetReal2D(A,1,1,1+5*smoothing);
        cvSetReal2D(A,numObs-2,numObs-2,1+5*smoothing);
        cvSetReal2D(A,numObs-2,numObs-1,-2*smoothing);
        cvSetReal2D(A,numObs-1,numObs-2,-2*smoothing);
        cvSetReal2D(A,numObs-1,numObs-1,1+smoothing);
    }
    CvMat* Trend_x= cvCreateMat(numObs,1,CV_32FC1);

    cvSolve(A,b,Trend_x);

    for (int i=0;i<Trend_x->rows;i++)
    {
        Trend[i]=Trend_x->data.fl[i];
    }
    cvReleaseMat(&Trend_x);
    cvReleaseMat(&b);
    cvReleaseMat(&A);
}
void hpfitler(std::deque<Point2Df>& pY, std::deque<Point2Df>& pTrend, double smoothing)
{
    if (pTrend.empty())
    {
        pTrend=pY;
    }
    int Ylength=pY.size();
    float *Y=new float[Ylength];
    float *Trend=new float[Ylength];

    for (int i=0;i<Ylength;i++)
        Y[i]=pY[i].x;
    hpfitler<float>(Y,Trend,Ylength,smoothing);
    for (int i=0;i<Ylength;i++)
        pTrend[i].x=Trend[i];

    for (int i=0;i<Ylength;i++)
        Y[i]=pY[i].y;
    hpfitler<float>(Y,Trend,Ylength,smoothing);
    for (int i=0;i<Ylength;i++)
        pTrend[i].y=Trend[i];

    delete[] Y;
    delete[] Trend;
}
void hpfitler(std::deque<Point3Df>& pY, std::deque<Point3Df>& pTrend, double smoothing)
{
    if (pTrend.empty())
    {
        pTrend=pY;
    }
    int Ylength=pY.size();
    float *Y=new float[Ylength];
    float *Trend=new float[Ylength];

    for (int i=0;i<Ylength;i++)
        Y[i]=pY[i].x;
    hpfitler<float>(Y,Trend,Ylength,smoothing);
    for (int i=0;i<Ylength;i++)
        pTrend[i].x=Trend[i];

    for (int i=0;i<Ylength;i++)
        Y[i]=pY[i].y;
    hpfitler<float>(Y,Trend,Ylength,smoothing);
    for (int i=0;i<Ylength;i++)
        pTrend[i].y=Trend[i];

    for (int i=0;i<Ylength;i++)
        Y[i]=pY[i].z;
    hpfitler<float>(Y, Trend, Ylength, smoothing);
    for (int i=0;i<Ylength;i++)
        pTrend[i].z=Trend[i];

    delete[] Y;
    delete[] Trend;
}

Motion_Input::Motion_Input()
{
    _GyrFilterBuffer.clear();
    _AccFilterBuffer.clear();
    _Cursor_acc_val = 1.0;
    _Cursor_gain_val = 1.0;  //16:9
    _Cursor_smooth_val = 20;
    _Cursor_speed_val = 1.0;
    _uinput_fd = 0;
    _filterNumber = 5;
}
Motion_Input::~Motion_Input()
{

}

bool Motion_Input::filter_raw_sensor_data(Point3Df *origin_AccData, Point3Df *origin_GyrData)
{
    //int filterNumber = 20;
    Point3Df AccData;
    Point3Df GyrData;

    this->copy_sensor_data(origin_AccData, &AccData);
    this->copy_sensor_data(origin_GyrData, &GyrData);

    _GyrFilterBuffer.push_back(GyrData);
    _AccFilterBuffer.push_back(AccData);

    if (_GyrFilterBuffer.size() > _filterNumber)
    {
        _GyrFilterBuffer.pop_front();
        std::deque<Point3Df> filter_points(_GyrFilterBuffer.size());
        hpfitler(_GyrFilterBuffer, filter_points, _Cursor_smooth_val);

        //this->_GyroData = filter_points[_filterNumber - 1];

        double gyr_mean = this->cal_gyr_mean();
        double val_xyz = GyrData.x * GyrData.x + GyrData.y * GyrData.y + GyrData.z * GyrData.z;

        double zoom_m = 1 / (1 + 1000 * exp(-gyr_mean));
        double rate_m = 1 / (1 + 10 * exp(-gyr_mean / 100));
        double rate_xyz = 1 / (1 + 10 * exp(-val_xyz / 200));
        double rate = MIN(rate_xyz / rate_m, 1.);

        GyrData.x = zoom_m * (GyrData.x * (1 - rate) + filter_points[_filterNumber - 1].x * rate);
        GyrData.y = zoom_m * (GyrData.y * (1 - rate) + filter_points[_filterNumber - 1].y * rate);
        GyrData.z = zoom_m * (GyrData.z * (1 - rate) + filter_points[_filterNumber - 1].z * rate);
    }

    if (_AccFilterBuffer.size() > _filterNumber)
    {
        _AccFilterBuffer.pop_front();
        std::deque<Point3Df> filter_points(_AccFilterBuffer.size());
        hpfitler(_AccFilterBuffer, filter_points, _Cursor_smooth_val);
        AccData = filter_points[_filterNumber - 1];
    }

    this->copy_sensor_data(&AccData, origin_AccData);
    this->copy_sensor_data(&GyrData, origin_GyrData);

    return true;
}
bool Motion_Input::calculate_cursor_delta_value( Point3Df AccData, Point3Df GyrData, Point2D *Cursor_delta)
{
    int y_offset, x_offset;
    //double scale_rate = 1. / 1000;

    AccData.x *= _Cursor_acc_val;
    AccData.y *= _Cursor_acc_val;
    AccData.z *= _Cursor_acc_val;
/*
    GyrData.x *= _Cursor_gain_val;
    GyrData.x *= _Cursor_gain_val;
    GyrData.x *= _Cursor_gain_val;

    double a_norm = GyrData.x * GyrData.x + GyrData.y * GyrData.y + GyrData.z * GyrData.z;
    double g_norm = AccData.x * AccData.x + AccData.y * AccData.y + AccData.z * AccData.z;
    scale_rate *= 1 / (1 + 20 * exp(-a_norm));
    scale_rate *= 1 / (1 + 20 * exp(-a_norm / 2));
*/

    double xz_force = sqrt(AccData.x * AccData.x + AccData.z * AccData.z);
    double x_sin = AccData.x / xz_force;
    double z_sin = AccData.z / xz_force;


    x_offset =(- x_sin * GyrData.x - z_sin * GyrData.z) *_Cursor_gain_val;
    y_offset =(x_sin * GyrData.z - z_sin * GyrData.x)*_Cursor_gain_val;

/*
    x_offset = scale_rate * (- x_sin * _GyroData.x - z_sin * _GyroData.z) * _ImageWidth;
    y_offset = scale_rate * (x_sin * _GyroData.z - z_sin * _GyroData.x) * _ImageWidth;
*/
    Cursor_delta->x = x_offset;
    Cursor_delta->y = y_offset;

    return true;
}
bool Motion_Input::Set_cursor_movement_acceleration(double Cursor_acc_val)
{

    _Cursor_acc_val = Cursor_acc_val;
    return true;
}

bool Motion_Input::Set_cursor_movement_gain(double Cursor_gain_val)
{

    _Cursor_gain_val = Cursor_gain_val;  //16:9
    return true;
}

bool Motion_Input::Set_cursor_movement_smoothness(int Cursor_smooth_val)
{

    _Cursor_smooth_val = Cursor_smooth_val;
    return true;
}

bool Motion_Input::Set_cursor_movement_speed(double Cursor_speed_val)
{

    _Cursor_speed_val = Cursor_speed_val;
    return true;
}
bool Motion_Input::Set_filter_number(int filter_number)
{

    _filterNumber = filter_number;
    _GyrFilterBuffer.clear();
    _AccFilterBuffer.clear();
    return true;
}
bool Motion_Input::copy_sensor_data(Point3Df *source_Data, Point3Df *Data)
{
    if (!source_Data || !Data )
        return false;

    Data->x = source_Data->x;
    Data->y = source_Data->y;
    Data->z = source_Data->z;

    return true;
}

double Motion_Input::cal_gyr_mean()
{
    int judge_len = _filterNumber;
    int buf_num =  _GyrFilterBuffer.size();
    int start_frame = buf_num - judge_len;
    if (buf_num < judge_len) return -1;

    Point3Df gyr_mean(0, 0, 0);

    for(int i = start_frame; i < buf_num; i++){
        gyr_mean.x += _GyrFilterBuffer[i].x;
        gyr_mean.y += _GyrFilterBuffer[i].y;
        gyr_mean.z += _GyrFilterBuffer[i].z;
    }
    gyr_mean.x /= judge_len;
    gyr_mean.y /= judge_len;
    gyr_mean.z /= judge_len;

    return gyr_mean.x * gyr_mean.x + gyr_mean.y * gyr_mean.y + gyr_mean.z * gyr_mean.z;
}
