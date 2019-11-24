using System.Runtime.InteropServices;
using System.Data.SqlTypes;

namespace CLRVM
{
    public class CLRVM
    {
        [DllImport("VirtualMachine_x64.dll", EntryPoint = "VMRun", CallingConvention = CallingConvention.StdCall)]
        public static extern bool VMRun(byte[] program);

        public static void VMRunUDF(SqlBinary byteCode)
        {
            VMRun(byteCode.Value);
        }
    }
}
