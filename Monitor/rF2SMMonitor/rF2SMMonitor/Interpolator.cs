/*
Implementation of Interpolator class.

Scratch pad for getting interpolation math right.

Author: The Iron Wolf (vleonavicius @hotmail.com)
*/
using rF2SMMonitor.rFactor2Data;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using static rF2SMMonitor.rFactor2Constants;

namespace rF2SMMonitor
{
  internal static class Interpolator
  {
    internal static double nlerpX = 0.0;
    internal static double nlerpY = 0.0;
    internal static double nlerpZ = 0.0;
    internal static double nlerpYaw = 0.0;
    internal static double nlerpSpeed = 0.0;

    internal static double matX = 0.0;
    internal static double matY = 0.0;
    internal static double matZ = 0.0;
    internal static double matYaw = 0.0;
    internal static double matSpeed = 0.0;

    internal static double slerpX = 0.0;
    internal static double slerpY = 0.0;
    internal static double slerpZ = 0.0;
    internal static double slerpYaw = 0.0;

    internal static double scoringX = 0.0;
    internal static double scoringY = 0.0;
    internal static double scoringZ = 0.0;
    internal static double scoringYaw = 0.0;

    internal struct Vec3
    {
      internal double x, y, z;
      internal Vec3(double x, double y, double z)
      { this.x = x; this.y = y; this.z = z; }

      internal void Rotate(Quaternion r)
      {
        double num12 = r.x + r.x;
        double num2 = r.y + r.y;
        double num = r.z + r.z;
        double num11 = r.w * num12;
        double num10 = r.w * num2;
        double num9 = r.w * num;
        double num8 = r.x * num12;
        double num7 = r.x * num2;
        double num6 = r.x * num;
        double num5 = r.y * num2;
        double num4 = r.y * num;
        double num3 = r.z * num;
        double num15 = ((x * ((1.0 - num5) - num3)) + (y * (num7 - num9))) + (z * (num6 + num10));
        double num14 = ((x * (num7 + num9)) + (y * ((1.0 - num8) - num3))) + (z * (num4 - num11));
        double num13 = ((x * (num6 - num10)) + (y * (num4 + num11))) + (z * ((1.0 - num8) - num5));
        x = num15;
        y = num14;
        z = num13;
      }
    }

    internal static void RenderDebugInfo(ref rF2State state, Graphics g)
    {
#if DEBUG_INTERPOLATION
      // Pick player veh
      var playerID = state.mID;
      if (state.mNumVehicles == 0)
      {
        return;
      }

      var delta = state.mDeltaTime;
      if (delta > 0.22)
        return; // Don't interpolate after pause

      var plrVeh = state.mVehicles[0];
      Debug.Assert(plrVeh.mID == playerID);
      // My
      var RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR = 0.01;
      var RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR = 0.03;
      var RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR = 0.3;

      var label = new StringBuilder();
      var data = new StringBuilder();

      // Save scoring pos:
      var euler = MathHelper.EulerFromOrientationMatrix(ref plrVeh.mOri);
      Interpolator.scoringYaw = euler.yaw;
      Interpolator.scoringX = plrVeh.mPosScoring.x;
      Interpolator.scoringY = plrVeh.mPosScoring.y;
      Interpolator.scoringZ = plrVeh.mPosScoring.z;

      // Do matrix interpolation
      Vec3 localRotAccel = new Vec3(plrVeh.mLocalRotAccel.x, plrVeh.mLocalRotAccel.y, plrVeh.mLocalRotAccel.z);
      Vec3 localAccel = new Vec3(plrVeh.mLocalAccel.x, plrVeh.mLocalAccel.y, plrVeh.mLocalAccel.z);
      Vec3 localRot = new Vec3(plrVeh.mLocalRot.x, plrVeh.mLocalRot.y, plrVeh.mLocalRot.z);
      Vec3 localVel = new Vec3(plrVeh.mLocalVel.x, plrVeh.mLocalVel.y, plrVeh.mLocalVel.z);

      localRot.x += localRotAccel.x * delta * RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR;
      localRot.y += localRotAccel.y * delta * RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR;
      localRot.z += localRotAccel.z * delta * RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR;

      localVel.x += localAccel.x * delta * RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR;
      localVel.y += localAccel.y * delta * RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR;
      localVel.z += localAccel.z * delta * RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR;


      Vec3 oriX = new Vec3( plrVeh.mOri[RowX].x, plrVeh.mOri[RowX].y, plrVeh.mOri[RowX].z );
      Vec3 oriY = new Vec3( plrVeh.mOri[RowY].x, plrVeh.mOri[RowY].y, plrVeh.mOri[RowY].z );
      Vec3 oriZ = new Vec3( plrVeh.mOri[RowZ].x, plrVeh.mOri[RowZ].y, plrVeh.mOri[RowZ].z );
      Vec3 wRot = new Vec3( ((oriX.x * localRot.x) + (oriX.y * localRot.y) + (oriX.z * localRot.z)) * delta * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR,
          ((oriY.x * localRot.x) + (oriY.y * localRot.y) + (oriY.z * localRot.z)) * delta * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR,
          ((oriZ.x * localRot.x) + (oriZ.y * localRot.y) + (oriZ.z * localRot.z)) * delta * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR);

      label.Append("Transformed rot:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", wRot.x, wRot.y, wRot.z);

      euler = MathHelper.EulerFromOrientationMatrix(ref plrVeh.mOri);

      label.Append("Euler original:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", euler.pitch, euler.yaw, euler.roll);

      Vec3 tmpX;
      Vec3 tmpY;
      Vec3 tmpZ;
      double tmpLen = 0.0;

      // rF2 world coordinates are left-handed, so all sine terms are negated
      // compared to rh system.

      // X
      // Rotate by z
      tmpZ.x = oriX.x * Math.Cos(wRot.z) - oriX.y * -Math.Sin(wRot.z);
      tmpZ.y = oriX.x * -Math.Sin(wRot.z) + oriX.y * Math.Cos(wRot.z);
      tmpZ.z = oriX.z;
      // Rotate by y
      tmpY.x = tmpZ.x * Math.Cos(wRot.y) + tmpZ.z * -Math.Sin(wRot.y);
      tmpY.y = tmpZ.y;
      tmpY.z = tmpZ.z * Math.Cos(wRot.y) - tmpZ.x * -Math.Sin(wRot.y);
      // Rotate by x
      tmpX.x = tmpY.x;
      tmpX.y = tmpY.y * Math.Cos(wRot.x) - tmpY.z * -Math.Sin(wRot.x);
      tmpX.z = tmpY.y * -Math.Sin(wRot.x) + tmpY.z * Math.Cos(wRot.x);
      tmpLen = Math.Sqrt(tmpX.x * tmpX.x + tmpX.y * tmpX.y + tmpX.z * tmpX.z);
      if (tmpLen > 0)
      {
        oriX = new Vec3(tmpX.x / tmpLen, tmpX.y / tmpLen, tmpX.z / tmpLen);
      }

      // Y
      // Rotate by z
      tmpZ.x = oriY.x * Math.Cos(wRot.z) - oriY.y * -Math.Sin(wRot.z);
      tmpZ.y = oriY.x * -Math.Sin(wRot.z) + oriY.y * Math.Cos(wRot.z);
      tmpZ.z = oriY.z;
      // Rotate by y
      tmpY.x = tmpZ.x * Math.Cos(wRot.y) + tmpZ.z * -Math.Sin(wRot.y);
      tmpY.y = tmpZ.y;
      tmpY.z = tmpZ.z * Math.Cos(wRot.y) - tmpZ.x * -Math.Sin(wRot.y);
      // Rotate by x
      tmpX.x = tmpY.x;
      tmpX.y = tmpY.y * Math.Cos(wRot.x) - tmpY.z * -Math.Sin(wRot.x);
      tmpX.z = tmpY.y * -Math.Sin(wRot.x) + tmpY.z * Math.Cos(wRot.x);
      tmpLen = Math.Sqrt(tmpX.x * tmpX.x + tmpX.y * tmpX.y + tmpX.z * tmpX.z);
      if (tmpLen > 0)
      {
        oriY = new Vec3(tmpX.x / tmpLen, tmpX.y / tmpLen, tmpX.z / tmpLen);
      }

      // Z
      // Rotate by z
      tmpZ.x = oriZ.x * Math.Cos(wRot.z) - oriZ.y * -Math.Sin(wRot.z);
      tmpZ.y = oriZ.x * -Math.Sin(wRot.z) + oriZ.y * Math.Cos(wRot.z);
      tmpZ.z = oriZ.z;
      // Rotate by y
      tmpY.x = tmpZ.x * Math.Cos(wRot.y) + tmpZ.z * -Math.Sin(wRot.y);
      tmpY.y = tmpZ.y;
      tmpY.z = tmpZ.z * Math.Cos(wRot.y) - tmpZ.x * -Math.Sin(wRot.y);
      // Rotate by x
      tmpX.x = tmpY.x;
      tmpX.y = tmpY.y * Math.Cos(wRot.x) - tmpY.z * -Math.Sin(wRot.x);
      tmpX.z = tmpY.y * -Math.Sin(wRot.x) + tmpY.z * Math.Cos(wRot.x);
      tmpLen = Math.Sqrt(tmpX.x * tmpX.x + tmpX.y * tmpX.y + tmpX.z * tmpX.z);
      if (tmpLen > 0)
      {
        oriZ = new Vec3(tmpX.x / tmpLen, tmpX.y / tmpLen, tmpX.z / tmpLen);
      }

      rF2Vec3[] rotatedOri = new rF2Vec3[3];
      rotatedOri[RowX].x = oriX.x; rotatedOri[RowX].y = oriX.y; rotatedOri[RowX].z = oriX.z;
      rotatedOri[RowY].x = oriY.x; rotatedOri[RowY].y = oriY.y; rotatedOri[RowY].z = oriY.z;
      rotatedOri[RowZ].x = oriZ.x; rotatedOri[RowZ].y = oriZ.y; rotatedOri[RowZ].z = oriZ.z;

      // NOTE: It looks like Yaw rotates negatively.
      euler = MathHelper.EulerFromOrientationMatrix(ref rotatedOri);
      label.Append("Euler rotated:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", euler.pitch, euler.yaw, euler.roll);

      var posMatrix = new Vec3(plrVeh.mPosScoring.x, plrVeh.mPosScoring.y, plrVeh.mPosScoring.z);

      posMatrix.x += ((oriX.x * localVel.x) + (oriX.y * localVel.y) + (oriX.z * localVel.z)) * delta;
      posMatrix.y += ((oriY.x * localVel.x) + (oriY.y * localVel.y) + (oriY.z * localVel.z)) * delta;
      posMatrix.z += ((oriZ.x * localVel.x) + (oriZ.y * localVel.y) + (oriZ.z * localVel.z)) * delta;

      localVel.x += localAccel.x * delta * 0.02;
      localVel.y += localAccel.y * delta * 0.02;
      localVel.z += localAccel.z * delta * 0.02;

      Interpolator.matX = posMatrix.x;
      Interpolator.matY = posMatrix.y;
      Interpolator.matZ = posMatrix.z;
      Interpolator.matYaw = Math.Atan2(oriZ.x, oriZ.z);
      Interpolator.matSpeed = Math.Sqrt((localVel.x * localVel.x) + (localVel.y * localVel.y) + (localVel.z * localVel.z));

      /////////////////////////////////////////////////////////////////////////////////////////////
      // Quaternion experiment

      // This will be in state
      Quaternion oriQuat = new Quaternion();
      oriQuat.ConvertMatToQuat(ref plrVeh.mOri);
      oriQuat.NormalizeIfNeeded();

      euler = MathHelper.EulerFromOrientationMatrix(ref plrVeh.mOri);

      Quaternion rotQuat = new Quaternion();
      rotQuat.EulerToQuat(wRot.x, wRot.y, wRot.z);
      rotQuat.NormalizeIfNeeded();

      rF2Vec3[] oriRestored = new rF2Vec3[3];
      rotQuat.ConvertQuatToMat(ref oriRestored);

      // TODO: figure out why yaw inverts.
      /*euler = MathHelper.EulerFromOrientationMatrix(ref oriRestored);
      label.Append("Euler trans rot quat:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", euler.pitch, euler.yaw, euler.roll);*/

      // q x r means rotate by q then by r.  In our case, rotate original rotation
      // by estimated rotation at this time.
      oriQuat.MultByQuat(rotQuat);
      oriQuat.NormalizeIfNeeded();

      rF2Vec3[] oriQuatRotated = new rF2Vec3[3];
      oriQuat.ConvertQuatToMat(ref oriQuatRotated);

      euler = MathHelper.EulerFromOrientationMatrix(ref oriQuatRotated);

      label.Append("Euler quat rotated:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", euler.pitch, euler.yaw, euler.roll);

      euler = MathHelper.EulerFromQuaternion(oriQuat);

      /*label.Append("Euler from quat:\n\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n\n", euler.pitch, euler.yaw, euler.roll);*/

      //
      // nlerp/slerp
      // Calculate initial orientation quaternion
      //
      oriQuat.ConvertMatToQuat(ref plrVeh.mOri);
      oriQuat.NormalizeIfNeeded();

      euler = MathHelper.EulerFromQuaternion(oriQuat);

      label.Append("\nEuler quat start:\n");
      data.AppendFormat("\nx {0:n5} y {1:n5} z {2:n5}\n", euler.pitch, euler.yaw, euler.roll);

      localRotAccel = new Vec3(plrVeh.mLocalRotAccel.x, plrVeh.mLocalRotAccel.y, plrVeh.mLocalRotAccel.z);
      localAccel = new Vec3(plrVeh.mLocalAccel.x, plrVeh.mLocalAccel.y, plrVeh.mLocalAccel.z);
      localRot = new Vec3(plrVeh.mLocalRot.x, plrVeh.mLocalRot.y, plrVeh.mLocalRot.z);
      localVel = new Vec3(plrVeh.mLocalVel.x, plrVeh.mLocalVel.y, plrVeh.mLocalVel.z);

      
      //var delta2 = 1.0; // Delta2 is used to calculate "end" quaternion. 
      // lack of means 1.0 (one second).

      // Estimate final speeds
      localRot.x += localRotAccel.x * RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR;
      localRot.y += localRotAccel.y * RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR;
      localRot.z += localRotAccel.z * RF2_SHARED_MEMORY_ACC_SMOOTH_FACTOR;

      localVel.x += localAccel.x * RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR;
      localVel.y += localAccel.y * RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR;
      localVel.z += localAccel.z * RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR;

      /*oriX = new Vec3(plrVeh.mOri[RowX].x, plrVeh.mOri[RowX].y, plrVeh.mOri[RowX].z);
      oriY = new Vec3(plrVeh.mOri[RowY].x, plrVeh.mOri[RowY].y, plrVeh.mOri[RowY].z);
      oriZ = new Vec3(plrVeh.mOri[RowZ].x, plrVeh.mOri[RowZ].y, plrVeh.mOri[RowZ].z);
      wRot = new Vec3(((oriX.x * localRot.x) + (oriX.y * localRot.y) + (oriX.z * localRot.z)) * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR,
          ((oriY.x * localRot.x) + (oriY.y * localRot.y) + (oriY.z * localRot.z)) * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR,
          ((oriZ.x * localRot.x) + (oriZ.y * localRot.y) + (oriZ.z * localRot.z)) * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR);

      label.Append("Transformed rot:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", wRot.x, wRot.y, wRot.z);*/

      // Convert local rotation to world rotation.
      wRot = new Vec3(localRot.x * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR,
        localRot.y * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR,
        localRot.z * RF2_SHARED_MEMORY_ROT_SMOOTH_FACTOR);

      wRot.Rotate(oriQuat);

      label.Append("Transformed rot:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", wRot.x, wRot.y, wRot.z);

      rotQuat = new Quaternion();
      rotQuat.EulerToQuat(wRot.x, wRot.y, wRot.z);
      rotQuat.NormalizeIfNeeded();

      // TRY REVERSING, measure delta.
      var oriQuatEnd = new Quaternion(oriQuat);
      oriQuatEnd.MultByQuat(rotQuat);
      oriQuatEnd.NormalizeIfNeeded();

      /*var oriQuatEnd = new Quaternion(rotQuat);
      oriQuatEnd.MultByQuat(oriQuat);
      oriQuatEnd.NormalizeIfNeeded();*/
      //var oriQuatEnd = rotQuat;

      euler = MathHelper.EulerFromQuaternion(oriQuatEnd);
      label.Append("Euler quat end:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", euler.pitch, euler.yaw, euler.roll);

      var nlerpQuat = MathHelper.Nlerp(oriQuat, oriQuatEnd, delta);

      euler = MathHelper.EulerFromQuaternion(nlerpQuat);
      label.AppendFormat("Euler quat nlerp (delta {0:n3}):\n", delta);
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", euler.pitch, euler.yaw, euler.roll);

      // ffs I don't get where the fuck problem is yaw not matching rF1 math, just negate, that's not a NASA drone.
      Interpolator.nlerpYaw = -euler.yaw;

      var slerpQuat = MathHelper.Slerp(oriQuat, oriQuatEnd, delta);

      euler = MathHelper.EulerFromQuaternion(slerpQuat);
      label.AppendFormat("Euler quat slerp (delta {0:n3}):\n", delta);
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", euler.pitch, euler.yaw, euler.roll);

      Interpolator.slerpYaw = -euler.yaw;

      var offset = new Vec3(localVel.x, localVel.y, localVel.z);
      offset.Rotate(nlerpQuat);

      var posNlerp = new Vec3(plrVeh.mPosScoring.x, plrVeh.mPosScoring.y, plrVeh.mPosScoring.z);
      posNlerp.x += offset.x * delta;
      posNlerp.y += offset.y * delta;
      posNlerp.z += offset.z * delta;

      Interpolator.nlerpX = posNlerp.x;
      Interpolator.nlerpY = posNlerp.y;
      Interpolator.nlerpZ = posNlerp.z;

      localAccel = new Vec3(plrVeh.mLocalAccel.x, plrVeh.mLocalAccel.y, plrVeh.mLocalAccel.z);
      localVel = new Vec3(plrVeh.mLocalVel.x, plrVeh.mLocalVel.y, plrVeh.mLocalVel.z);

      RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR = 1.0;
      localVel.x += localAccel.x * delta * RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR;
      localVel.y += localAccel.y * delta * RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR;
      localVel.z += localAccel.z * delta * RF2_SHARED_MEMORY_VEL_ACC_SMOOTH_FACTOR;
      Interpolator.nlerpSpeed = Math.Sqrt((localVel.x * localVel.x) + (localVel.y * localVel.y) + (localVel.z * localVel.z));



      offset = new Vec3(localVel.x, localVel.y, localVel.z);
      offset.Rotate(slerpQuat);

      var posSlerp = new Vec3(plrVeh.mPosScoring.x, plrVeh.mPosScoring.y, plrVeh.mPosScoring.z);
      posSlerp.x += offset.x * delta;
      posSlerp.y += offset.y * delta;
      posSlerp.z += offset.z * delta;

      Interpolator.slerpX = posSlerp.x;
      Interpolator.slerpY = posSlerp.y;
      Interpolator.slerpZ = posSlerp.z;

      var oriNlerp = new rF2Vec3[3];
      nlerpQuat.ConvertQuatToMat(ref oriNlerp);

      var posNlerpMat = new Vec3(plrVeh.mPosScoring.x, plrVeh.mPosScoring.y, plrVeh.mPosScoring.z);
      posNlerpMat.x += ((oriNlerp[RowX].x * localVel.x) + (oriNlerp[RowX].y * localVel.y) + (oriNlerp[RowX].z * localVel.z)) * delta;
      posNlerpMat.y += ((oriNlerp[RowY].x * localVel.x) + (oriNlerp[RowY].y * localVel.y) + (oriNlerp[RowY].z * localVel.z)) * delta;
      posNlerpMat.z += ((oriNlerp[RowZ].x * localVel.x) + (oriNlerp[RowZ].y * localVel.y) + (oriNlerp[RowZ].z * localVel.z)) * delta;

      label.Append("\nPos real:\n");
      data.AppendFormat("\nx {0:n5} y {1:n5} z {2:n5}\n", state.mPos.x, state.mPos.y, state.mPos.z);

      label.Append("Pos scoring:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", plrVeh.mPosScoring.x, plrVeh.mPosScoring.y, plrVeh.mPosScoring.z);

      label.Append("Pos interpolated:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", posMatrix.x, posMatrix.y, posMatrix.z);

      label.Append("Pos nlerp:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", posNlerp.x, posNlerp.y, posNlerp.z);

      label.Append("Pos nlerp mat:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", posNlerpMat.x, posNlerpMat.y, posNlerpMat.z);

      label.Append("Pos slerp:\n");
      data.AppendFormat("x {0:n5} y {1:n5} z {2:n5}\n", posSlerp.x, posSlerp.y, posSlerp.z);

      g.DrawString(label.ToString(), SystemFonts.DefaultFont, Brushes.Green, 1250.0f, 3.0f);
      g.DrawString(data.ToString(), SystemFonts.DefaultFont, Brushes.Green, 1400.0f, 3.0f);
#endif
    }
  }
}

