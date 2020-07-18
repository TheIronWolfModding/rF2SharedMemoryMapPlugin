/*
Send HW controls to rFactor 2 via the shared memory.

Author: Tony Whitley (sven.smiles@gmail.com)
*/
using System;
using System.Text;

using rF2SharedMemory;
using rF2SharedMemory.rFactor2Data;
using static rF2SharedMemory.rFactor2Constants;

namespace rF2SharedMemory
{
    /// <summary>
    /// Provides a null for the attribute used in rF2Data.cs
    /// </summary>
    public class JsonIgnoreAttribute : Attribute
    {
    }

    /// <summary>
    /// Send HW controls to rF2 via the shared memory
    /// </summary>
    public class SendrF2HWControl
    {
        #region Private Fields

        // Write buffers:
        private readonly MappedBuffer<rF2HWControl> hwControlBuffer = new MappedBuffer<rF2HWControl>(rF2SharedMemory.rFactor2Constants.MM_HWCONTROL_FILE_NAME);

        // Marshalled output views:
        private rF2HWControl hwControl;

        private bool Connected = false;

        #endregion Private Fields

        #region Public Methods

        /// <summary>
        /// Connect to the Shared Memory running in rFactor
        /// </summary>
        /// <returns>
        /// true if connected
        /// </returns>
        public bool Connect()
        {
            if (!this.Connected)
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
            }
            return this.Connected;
        }

        /// <summary>
        /// Disconnect from the Shared Memory running in rFactor
        /// </summary>
        public void Disconnect()
        {
            this.hwControlBuffer.Disconnect();
        }

        /// <summary>
        /// Send the HW Control to rFactor via the Shared Memory plugin
        /// </summary>
        /// <param name="controlStr">
        /// The name used by rFactor.  Note that a lot of controls don't
        /// actually do anything, particularly none of the "driving controls"
        /// </param>
        /// <param name="down">
        /// Whether to set "mfRetVal" to 1.  What that does is not documented.
        /// </param>
        public void SendHWControl(string controlStr, bool down)
        {
            if (this.Connected && controlStr != null)
            {
                byte[] temp = Encoding.Default.GetBytes(controlStr);
                this.hwControl.mVersionUpdateBegin = this.hwControl.mVersionUpdateEnd = this.hwControl.mVersionUpdateBegin + 1;
                this.hwControl.mLayoutVersion = rFactor2Constants.MM_HWCONTROL_LAYOUT_VERSION;

                this.hwControl.mControlName = new byte[rFactor2Constants.MAX_HWCONTROL_NAME_LEN];
                for (int i = 0; i < controlStr.Length; ++i)
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

        #endregion Public Methods
    }
}