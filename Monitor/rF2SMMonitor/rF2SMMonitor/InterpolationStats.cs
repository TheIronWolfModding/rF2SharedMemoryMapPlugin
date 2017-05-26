/*
Implementation of InterpolationStats class.

Tracks and renders stats on how close interpolated values are to real
values by comparing player vehicle telemetry values to ScoringUpdate values.

Author: The Iron Wolf (vleonavicius@hotmail.com)
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
  internal class InterpolationStats
  {
    double xDeltaMax = 0.0;
    double yDeltaMax = 0.0;
    double zDeltaMax = 0.0;
    double distDeltaMax = 0.0;
    double distDeltaTotal = 0.0;
#if DEBUG_INTERPOLATION
    double distNlerpDeltaMax = 0.0;
    double distNlerpDeltaTotal = 0.0;
    double distScoringDeltaMax = 0.0;
    double distScoringDeltaTotal = 0.0;
#endif
    double speedDeltaMax = 0.0;
    double speedDeltaTotal = 0.0;
    double yawDeltaMax = 0.0;
    double yawDeltaTotal = 0.0;
    double rollDeltaMax = 0.0;
    double rollDeltaTotal = 0.0;
    double pitchDeltaMax = 0.0;
    double pitchDeltaTotal = 0.0;

    int updateCounter = 1;

    internal void Reset()
    {
      this.xDeltaMax = 0.0;
      this.yDeltaMax = 0.0;
      this.zDeltaMax = 0.0;
      this.distDeltaMax = 0.0;
      this.distDeltaTotal = 0.0;
#if DEBUG_INTERPOLATION
      this.distNlerpDeltaMax = 0.0;
      this.distNlerpDeltaTotal = 0.0;
      this.distScoringDeltaMax = 0.0;
      this.distScoringDeltaTotal = 0.0;
#endif
      this.speedDeltaMax = 0.0;
      this.speedDeltaTotal = 0.0;
      this.yawDeltaMax = 0.0;
      this.yawDeltaTotal = 0.0;
      this.rollDeltaMax = 0.0;
      this.rollDeltaTotal = 0.0;
      this.pitchDeltaMax = 0.0;
      this.pitchDeltaTotal = 0.0;

      this.updateCounter = 1;
    }

    internal void RenderInterpolationInfo(ref rFactor2Data.rF2State currrF2State, Graphics g, int currBuff)
    {
      //var playerName = ASCIIEncoding.ASCII.GetString(this.currrF2State.mPlayerName);
      var playerID = currrF2State.mID;
      if (currrF2State.mNumVehicles == 0)
      {
        this.Reset();
        return;
      }

      ++this.updateCounter;

      var playerVeh = currrF2State.mVehicles[0];
      Debug.Assert(playerID == playerVeh.mID);

      var oriReal = currrF2State.mOri;
      var yawReal = Math.Atan2(oriReal[RowZ].x, oriReal[RowZ].z);
      var yawInterpolated = playerVeh.mYaw;
      
      var pitchReal = Math.Atan2(-oriReal[RowY].z,
        Math.Sqrt(oriReal[RowX].z * oriReal[RowX].z + oriReal[RowZ].z * oriReal[RowZ].z));
      var pitchInterpolated = playerVeh.mPitch;
      
      var rollReal = Math.Atan2(oriReal[RowY].x,
        Math.Sqrt(oriReal[RowX].x * oriReal[RowX].x + oriReal[RowZ].x * oriReal[RowZ].x));
      var rollInterpolated = playerVeh.mRoll;

      var posReal = currrF2State.mPos;
      var posInterpolated = playerVeh.mPos;

      var posNlerpInterpolated = new rF2Vec3();
      posNlerpInterpolated.x = Interpolator.nlerpX;
      posNlerpInterpolated.y = Interpolator.nlerpY;
      posNlerpInterpolated.z = Interpolator.nlerpZ;

      var speedReal = currrF2State.mSpeed;
      var speedInterpolated = playerVeh.mSpeed;

      var lapDistInterpolated = playerVeh.mLapDist;

      var label = new StringBuilder();
      var data = new StringBuilder();

      // Position
      label.Append("Real Pos:\n");
      data.AppendFormat("x {0:n3} y {1:n3} z {2:n3}\n", posReal.x, posReal.y, posReal.z);
      label.Append("Inter Pos:\n");
      data.AppendFormat("x {0:n3} y {1:n3} z {2:n3}\n", posInterpolated.x, posInterpolated.y, posInterpolated.z);
      label.Append("Pos Delta:\n");
      var xDelta = Math.Abs(posReal.x - posInterpolated.x);
      var yDelta = Math.Abs(posReal.y - posInterpolated.y);
      var zDelta = Math.Abs(posReal.z - posInterpolated.z);
      data.AppendFormat("x {0:n3} y {1:n3} z {2:n3}\n", xDelta, yDelta, zDelta);

      label.Append("Max nlerp Pos Delta:\n");
      this.xDeltaMax = Math.Max(xDeltaMax, xDelta);
      this.yDeltaMax = Math.Max(yDeltaMax, yDelta);
      this.zDeltaMax = Math.Max(zDeltaMax, zDelta);
      data.AppendFormat("x {0:n3} y {1:n3} z {2:n3}\n", this.xDeltaMax, this.yDeltaMax, this.zDeltaMax);

      label.Append("\nDelta nlerp Dist:\n");
      var deltaDist = MathHelper.Distance(ref posReal, ref posInterpolated);
      data.AppendFormat("\n{0:n3} meters\n", deltaDist);
      label.Append("Max nlerp Dist Delta:\n");
      this.distDeltaMax = Math.Max(this.distDeltaMax, deltaDist);
      data.AppendFormat("{0:n3} meters\n", this.distDeltaMax);
      label.Append("Avg nlerp Dist Delta:\n\n");
      this.distDeltaTotal += deltaDist;
      data.AppendFormat("{0:n3} meters\n\n", this.distDeltaTotal / this.updateCounter);

#if DEBUG_INTERPOLATION
      // nlerp dist deltas
      label.Append("Delta Nlerp Dist:\n");
      deltaDist = MathHelper.Distance(ref posReal, ref posNlerpInterpolated);
      data.AppendFormat("{0:n3} meters\n", deltaDist);
      label.Append("Max Nlerp Dist Delta:\n");
      this.distNlerpDeltaMax = Math.Max(this.distNlerpDeltaMax, deltaDist);
      data.AppendFormat("{0:n3} meters\n", this.distNlerpDeltaMax);
      label.Append("Avg Nlerp Dist Delta:\n\n");
      this.distNlerpDeltaTotal += deltaDist;
      data.AppendFormat("{0:n3} meters\n\n", this.distNlerpDeltaTotal / this.updateCounter);

      //Debug.Print("dist: {0:n3} dt: {1:n3} buff: {2}", deltaDist, currrF2State.mDeltaTime, currBuff);

      var posScoring = new rF2Vec3();
      posScoring.x = Interpolator.scoringX;
      posScoring.y = Interpolator.scoringY;
      posScoring.z = Interpolator.scoringZ;

      // scoring dist deltas (shows what'll happen if we don't interpolate.)
      label.Append("Delta Scoring Dist:\n");
      deltaDist = MathHelper.Distance(ref posReal, ref posScoring);
      data.AppendFormat("{0:n3} meters\n", deltaDist);
      label.Append("Max Scoring Dist Delta:\n");
      this.distScoringDeltaMax = Math.Max(this.distScoringDeltaMax, deltaDist);
      data.AppendFormat("{0:n3} meters\n", this.distScoringDeltaMax);
      label.Append("Avg Scoring Dist Delta:\n\n");
      this.distScoringDeltaTotal += deltaDist;
      data.AppendFormat("{0:n3} meters\n\n", this.distScoringDeltaTotal / this.updateCounter);
      //Debug.Print("dist: {0:n3} dt: {1:n3} buff: {2}", deltaDist, currrF2State.mDeltaTime, currBuff);
#endif
      // Speed
      label.Append("Real Speed:\n");
      data.AppendFormat("{0:n3} m/s {1:n4} km/h\n", speedReal, speedReal * 3.6);
      label.Append("Inter Speed:\n");
      data.AppendFormat("{0:n3} m/s {1:n4} km/h\n", speedInterpolated, speedInterpolated * 3.6);
      label.Append("Speed Delta:\n");
      var speedDelta = Math.Abs(speedReal - speedInterpolated);
      data.AppendFormat("{0:n3} m/s\n", speedDelta);
      label.Append("Max Speed Delta:\n");
      this.speedDeltaMax = Math.Max(this.speedDeltaMax, speedDelta);
      data.AppendFormat("{0:n3} m/s\n", this.speedDeltaMax);
      label.Append("Avg Speed Delta:\n\n");
      this.speedDeltaTotal += speedDelta;
      data.AppendFormat("{0:n3} m/s\n\n", this.speedDeltaTotal / this.updateCounter);

      // yaw
      label.Append("Real yaw:\n");
      data.AppendFormat("{0:n3} radians\n", yawReal);
      label.Append("Inter yaw:\n");
      data.AppendFormat("{0:n3} radians\n", yawInterpolated);
      label.Append("yaw Delta:\n");
      var yawDelta = Math.Abs(yawReal - yawInterpolated);
      data.AppendFormat("{0:n3} radians\n", yawDelta);
      label.Append("Max yaw Delta:\n");
      this.yawDeltaMax = Math.Max(this.yawDeltaMax, yawDelta);
      data.AppendFormat("{0:n3} radians\n", this.yawDeltaMax);
      label.Append("Avg yaw Delta:\n\n");
      this.yawDeltaTotal += yawDelta;
      data.AppendFormat("{0:n3} radians\n\n", this.yawDeltaTotal / this.updateCounter);

      // roll
      label.Append("Real roll:\n");
      data.AppendFormat("{0:n3} radians\n", rollReal);
      label.Append("Inter roll:\n");
      data.AppendFormat("{0:n3} radians\n", rollInterpolated);
      label.Append("roll Delta:\n");
      var rollDelta = Math.Abs(rollReal - rollInterpolated);
      data.AppendFormat("{0:n3} radians\n", rollDelta);
      label.Append("Max roll Delta:\n");
      this.rollDeltaMax = Math.Max(this.rollDeltaMax, rollDelta);
      data.AppendFormat("{0:n3} radians\n", this.rollDeltaMax);
      label.Append("Avg roll Delta:\n\n");
      this.rollDeltaTotal += rollDelta;
      data.AppendFormat("{0:n3} radians\n\n", this.rollDeltaTotal / this.updateCounter);

      // pitch
      label.Append("Real pitch:\n");
      data.AppendFormat("{0:n3} radians\n", pitchReal);
      label.Append("Inter pitch:\n");
      data.AppendFormat("{0:n3} radians\n", pitchInterpolated);
      label.Append("pitch Delta:\n");
      var pitchDelta = Math.Abs(pitchReal - pitchInterpolated);
      data.AppendFormat("{0:n3} radians\n", pitchDelta);
      label.Append("Max pitch Delta:\n");
      this.pitchDeltaMax = Math.Max(this.pitchDeltaMax, pitchDelta);
      data.AppendFormat("{0:n3} radians\n", this.pitchDeltaMax);
      label.Append("Avg pitch Delta:\n\n");
      this.pitchDeltaTotal += pitchDelta;
      data.AppendFormat("{0:n3} radians\n\n", this.pitchDeltaTotal / this.updateCounter);

      g.DrawString(label.ToString(), SystemFonts.DefaultFont, Brushes.Green, 1600.0f, 3.0f);
      g.DrawString(data.ToString(), SystemFonts.DefaultFont, Brushes.Green, 1720.0f, 3.0f);
    }
  }
}
