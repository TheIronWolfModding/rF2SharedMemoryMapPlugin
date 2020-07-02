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
    MappedBuffer<rF2HWControl> hwControlBuffer = new MappedBuffer<rF2HWControl>(rF2SharedMemory.rFactor2Constants.MM_HWCONTROL_FILE_NAME);

    // Marshalled output views:
    rF2HWControl hwControl;

    private bool Connected = false;

    public bool Connect()
    {
      try
      {
        this.hwControlBuffer.Connect();
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
        this.hwControl.mVersionUpdateBegin = this.hwControl.mVersionUpdateEnd = this.hwControl.mVersionUpdateBegin + 1;
        this.hwControl.mLayoutVersion = rFactor2Constants.MM_HWCONTROL_LAYOUT_VERSION;

        this.hwControl.mControlName = new byte[rFactor2Constants.MAX_HWCONTROL_NAME_LEN];
        for (int i = 0; i < commandStr.Length; ++i)
          this.hwControl.mControlName[i] = temp[i];

        if (down)
        {
          this.hwControl.mfRetVal = 1.0;
        }
        else
        {
          this.hwControl.mfRetVal = 0.0;
        }

        this.hwControlBuffer.PutMappedData(ref this.hwControl);
      }
    }
  }
}
