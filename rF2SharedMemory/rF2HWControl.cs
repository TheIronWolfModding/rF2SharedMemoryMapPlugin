using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using rF2SharedMemory;
using rF2SharedMemory.rFactor2Data;
using static rF2SharedMemory.rFactor2Constants;

namespace rF2SharedMemory
{
  public class SendrF2HWControl
  {
    // Write buffers:
    MappedBuffer<rF2HWControl> hwcontrolBuffer = new MappedBuffer<rF2HWControl>(rF2SharedMemory.rFactor2Constants.MM_HWCONTROL_FILE_NAME);

    // Marshalled output views:
    rF2HWControl hwcontrol;

    private bool Connected = false;

    public bool Connect()
    {
      try
      {
        this.hwcontrolBuffer.Connect();
        this.Connected = true;
      }
      catch (Exception)
      {
        this.Connected = false;
      }
      return this.Connected;
    }

    public void SendHWControl(string commandStr, bool down)
    {
      if (this.Connected && commandStr != null)
      {
        byte[] temp = Encoding.Default.GetBytes(commandStr);
        this.hwcontrol.mVersionUpdateBegin = this.hwcontrol.mVersionUpdateEnd = this.hwcontrol.mVersionUpdateBegin + 1;

        this.hwcontrol.mControlName = new byte[rFactor2Constants.MAX_HWCONTROL_NAME_LEN];
        for (int i = 0; i < temp.Length; ++i)
          this.hwcontrol.mControlName[i] = temp[i];

        if (down)
        {
          this.hwcontrol.mfRetVal = 1.0;
        }
        else
        {
          this.hwcontrol.mfRetVal = 0.0;
        }

        this.hwcontrolBuffer.PutMappedData(ref this.hwcontrol);
      }
    }
  }
}
