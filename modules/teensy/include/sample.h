#ifndef SAMPLE_H_
#define SAMPLE_H_

struct ImuSample
{
  float qx, qy, qz, qw;
  float gx, gy, gz;
  float ax, ay, az;
};

#endif
