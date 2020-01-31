using System.Runtime.InteropServices;
using System.Data.SqlTypes;

namespace CLRVM
{
    public class CLRVM
    {
        [DllImport("VirtualMachine_x64.dll", EntryPoint = "Run", CallingConvention = CallingConvention.StdCall)]
        public static extern void Run(byte[] program);
        
        [DllImport("VirtualMachine_x64.dll", EntryPoint = "NumRun", CallingConvention = CallingConvention.StdCall)]
        public static extern double NumRun(byte[] program);
        
        [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun", CallingConvention = CallingConvention.StdCall)]
        public static extern void StrRun(byte[] program, char[] res);

        public static void RunUDF(SqlBinary byteCode)
        {
            Run(byteCode.Value);
        }

        public static SqlDouble NumRunUDF(SqlBinary byteCode)
        {
            return NumRun(byteCode.Value);
        }

        public static SqlString StrRunUDF(SqlBinary byteCode)
        {
            char[] res = new char[100];
            StrRun(byteCode.Value, res);
            return new string(res);
        }
    }
}
