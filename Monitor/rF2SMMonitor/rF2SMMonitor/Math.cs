/*
Implementation of MathHelper and related classes.

Author: The Iron Wolf (vleonavicius@hotmail.com)
Website: thecrewchief.org
*/
using rF2SMMonitor.rFactor2Data;
using System;
using System.Diagnostics;
using static rF2SMMonitor.rFactor2Constants;

namespace rF2SMMonitor
{
  static internal class MathHelper
  {
    internal class EulerAngles
    {
      internal double yaw = 0.0;
      internal double pitch = 0.0;
      internal double roll = 0.0;

      internal EulerAngles(double yaw = 0.0, double roll = 0.0, double pitch = 0.0)
      {
        this.yaw = yaw;
        this.roll = roll;
        this.pitch = pitch;
      }
    }

    static internal double Distance(ref rF2Vec3 v1, ref rF2Vec3 v2)
    {
      return Math.Sqrt((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y) + (v1.z - v2.z) * (v1.z - v2.z));
    }

    static internal EulerAngles EulerFromOrientationMatrix(ref rF2Vec3[] oriMat)
    {
      Debug.Assert(oriMat.Length == 3);

      // From rf plugin
      var pitch = Math.Atan2(-oriMat[RowY].z, Math.Sqrt(oriMat[RowX].z * oriMat[RowX].z + oriMat[RowZ].z * oriMat[RowZ].z));
      var yaw = Math.Atan2(oriMat[RowZ].x, oriMat[RowZ].z);
      var roll = Math.Atan2(oriMat[RowY].x, Math.Sqrt(oriMat[RowX].x * oriMat[RowX].x + oriMat[RowZ].x * oriMat[RowZ].x));

      return new EulerAngles(yaw, roll, pitch);
    }

    static internal EulerAngles EulerFromQuaternion(Quaternion quat)
    {
      double test = quat.x * quat.y + quat.z * quat.w;
      double yaw, pitch, roll;
      if (test > 0.499)
      { // singularity at north pole
        yaw = 2.0 * Math.Atan2(quat.x, quat.w);
        roll = Math.PI / 2.0;
        pitch = 0.0;
        return new EulerAngles(yaw, roll, pitch);
      }
      else if (test < -0.499)
      { // singularity at south pole
        yaw = -2.0 * Math.Atan2(quat.x, quat.w);
        roll = -Math.PI / 2.0;
        pitch = 0.0;
        return new EulerAngles(yaw, roll, pitch);
      }

      double sqx = quat.x * quat.x;
      double sqy = quat.y * quat.y;
      double sqz = quat.z * quat.z;
      yaw = Math.Atan2(2.0 * quat.y * quat.w - 2.0 * quat.x * quat.z, 1.0 - 2.0 * sqy - 2.0 * sqz);
      roll = Math.Asin(2.0 * test);
      pitch = Math.Atan2(2.0 * quat.x * quat.w - 2.0 * quat.y * quat.z, 1.0 - 2.0 * sqx - 2.0 * sqz);

      return new EulerAngles(yaw, roll, pitch);
    }

    internal static Quaternion Nlerp(Quaternion a, Quaternion b, double t/*, bool reduceTo360*/)
    {
      double t1 = 1.0 - t;

      var r = new Quaternion();

      r.x = t1 * a.x + t * b.x;
      r.y = t1 * a.y + t * b.y;
      r.z = t1 * a.z + t * b.z;
      r.w = t1 * a.w + t * b.w;

      r.NormalizeIfNeeded();
      return r;
    }

    internal static Quaternion Slerp(Quaternion qa, Quaternion qb, double t)
    {
      // quaternion to return
      var qm = new Quaternion();
      // Calculate angle between them.
      double cosHalfTheta = qa.w * qb.w + qa.x * qb.x + qa.y * qb.y + qa.z * qb.z;
      // if qa=qb or qa=-qb then theta = 0 and we can return qa
      if (Math.Abs(cosHalfTheta) >= 1.0)
      {
        qm.w = qa.w; qm.x = qa.x; qm.y = qa.y; qm.z = qa.z;
        return qm;
      }
      // Calculate temporary values.
      double halfTheta = Math.Acos(cosHalfTheta);
      double sinHalfTheta = Math.Sqrt(1.0 - cosHalfTheta * cosHalfTheta);
      // if theta = 180 degrees then result is not fully defined
      // we could rotate around any axis normal to qa or qb
      if (Math.Abs(sinHalfTheta) < 0.001)
      { // fabs is floating point absolute
        qm.w = (qa.w * 0.5 + qb.w * 0.5);
        qm.x = (qa.x * 0.5 + qb.x * 0.5);
        qm.y = (qa.y * 0.5 + qb.y * 0.5);
        qm.z = (qa.z * 0.5 + qb.z * 0.5);
        qm.NormalizeIfNeeded();
        return qm;
      }
      double ratioA = Math.Sin((1 - t) * halfTheta) / sinHalfTheta;
      double ratioB = Math.Sin(t * halfTheta) / sinHalfTheta;
      //calculate Quaternion.
      qm.w = (qa.w * ratioA + qb.w * ratioB);
      qm.x = (qa.x * ratioA + qb.x * ratioB);
      qm.y = (qa.y * ratioA + qb.y * ratioB);
      qm.z = (qa.z * ratioA + qb.z * ratioB);

      qm.NormalizeIfNeeded();
      return qm;
      /*var r = new Quaternion();
      double t1 = 1.0 - t;
      double theta = Math.Acos(a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w);
      double sn = Math.Sin(theta);
      double Wa = Math.Sin(t1 * theta) / sn;
      double Wb = Math.Sin(t * theta) / sn;
      r.x = Wa* a.x + Wb* b.x;
      r.y = Wa* a.y + Wb* b.y;
      r.z = Wa* a.z + Wb* b.z;
      r.w = Wa* a.w + Wb* b.w;
      
      return r;*/
    }
  }

  internal class Quaternion
  {
    internal double w, x, y, z;

    internal Quaternion() { }

    internal Quaternion(Quaternion q)
    {
      this.w = q.w;
      this.x = q.x;
      this.y = q.y;
      this.z = q.z;
    }

    // Convert this quaternion to a matrix
    internal void ConvertQuatToMat(ref rF2Vec3[] ori)
     {
      double x2 = x + x;
      double xx = x * x2;
      double y2 = y + y;
      double yy = y * y2;
      double z2 = z + z;
      double zz = z * z2;
      double xz = x * z2;
      double xy = x * y2;
      double wy = w * y2;
      double wx = w * x2;
      double wz = w * z2;
      double yz = y * z2;
      ori[0].x = (double) 1.0 - ( yy + zz );
      ori[0].y = xy - wz;
      ori[0].z = xz + wy;
      ori[1].x = xy + wz;
      ori[1].y = (double) 1.0 - ( xx + zz );
      ori[1].z = yz - wx;
      ori[2].x = xz - wy;
      ori[2].y = yz + wx;
      ori[2].z = (double) 1.0 - ( xx + yy );
    }

    // Convert a matrix to this quaternion
    internal void ConvertMatToQuat(ref rF2Vec3[] ori)
    {
      double trace = ori[0].x + ori[1].y + ori[2].z + (double)1.0;
      if (trace > 0.0625f)
      {
        double sqrtTrace = Math.Sqrt(trace);
        double s = (double)0.5 / sqrtTrace;
        w = (double)0.5 * sqrtTrace;
        x = (ori[2].y - ori[1].z) * s;
        y = (ori[0].z - ori[2].x) * s;
        z = (ori[1].x - ori[0].y) * s;
      }
      else if ((ori[0].x > ori[1].y) && (ori[0].x > ori[2].z))
      {
        double sqrtTrace = Math.Sqrt((double)1.0 + ori[0].x - ori[1].y - ori[2].z);
        double s = (double)0.5 / sqrtTrace;
        w = (ori[2].y - ori[1].z) * s;
        x = (double)0.5 * sqrtTrace;
        y = (ori[0].y + ori[1].x) * s;
        z = (ori[0].z + ori[2].x) * s;
      }
      else if (ori[1].y > ori[2].z)
      {
        double sqrtTrace = Math.Sqrt((double)1.0 + ori[1].y - ori[0].x - ori[2].z);
        double s = (double)0.5 / sqrtTrace;
        w = (ori[0].z - ori[2].x) * s;
        x = (ori[0].y + ori[1].x) * s;
        y = (double)0.5 * sqrtTrace;
        z = (ori[1].z + ori[2].y) * s;
      }
      else
      {
        double sqrtTrace = Math.Sqrt((double)1.0 + ori[2].z - ori[0].x - ori[1].y);
        double s = (double)0.5 / sqrtTrace;
        w = (ori[1].x - ori[0].y) * s;
        x = (ori[0].z + ori[2].x) * s;
        y = (ori[1].z + ori[2].y) * s;
        z = (double)0.5 * sqrtTrace;
      }
    }

    // http://www.euclideanspace.com/maths/geometry/rotations/conversions/eulerToQuaternion/
    internal void EulerToQuat(double pitch, double yaw, double roll)
    {
      double c1 = Math.Cos(yaw * 0.5);
      double s1 = Math.Sin(yaw * 0.5);
      double c2 = Math.Cos(roll * 0.5);
      double s2 = Math.Sin(roll * 0.5);
      double c3 = Math.Cos(pitch * 0.5);
      double s3 = Math.Sin(pitch * 0.5);
      double c1c2 = c1 * c2;
      double s1s2 = s1 * s2;
      w = c1c2 * c3 - s1s2 * s3;
      x = c1c2 * s3 + s1s2 * c3;
      y = s1 * c2 * c3 + c1 * s2 * s3;
      z = c1 * s2 * c3 - s1 * c2 * s3;
    }

    // Multiply this * quat
    internal void MultByQuat(Quaternion quat)
    {
      //https://www.mathworks.com/help/aeroblks/quaternionmultiplication.html
      //t=q×r=t0+it1+jt2+kt3
      //t0=(r0q0−r1q1−r2q2−r3q3)
      //t1=(r0q1+r1q0−r2q3+r3q2)
      //t2=(r0q2+r1q3+r2q0−r3q1)
      //t3=(r0q3−r1q2+r2q1+r3q0)
      //
      // Meaning: rotate by q then by r.

      double r0 = quat.w;
      double r1 = quat.x;
      double r2 = quat.y;
      double r3 = quat.z;

      double q0 = w;
      double q1 = x;
      double q2 = y;
      double q3 = z;

      double t0 = r0*q0 - r1*q1 - r2*q2 - r3*q3;
      double t1 = r0*q1 + r1*q0 - r2*q3 + r3*q2;
      double t2 = r0*q2 + r1*q3 + r2*q0 - r3*q1;
      double t3 = r0*q3 - r1*q2 + r2*q1 + r3*q0;

      w = t0;
      x = t1;
      y = t2;
      z = t3;
    }


    internal void NormalizeIfNeeded()
    {
      double square = w * w + x * x + y * y + z * z;
      if (Math.Abs(square - 1.0) > 1e-6)
      {
        double magnitude = Math.Sqrt(square);
        w /= magnitude;
        x /= magnitude;
        y /= magnitude;
        z /= magnitude;
      }
    }

    // Can be used to determine Quaternion neighbourhood
    internal double Dot(Quaternion q)
    {
      return x * q.x +  y * q.y + z * q.z + w * q.w;
    }
  }
}
