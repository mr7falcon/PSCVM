using System.Runtime.InteropServices;
using System.Data.SqlTypes;

namespace CLRVM
{
    public class CLRVM
    {
        [DllImport("VirtualMachine_x64.dll", EntryPoint = "Run0", CallingConvention = CallingConvention.StdCall)]
        public static extern void Run0(byte[] program);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "Run1", CallingConvention = CallingConvention.StdCall)]
        public static extern void Run1(byte[] program, byte[] arg0);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "Run2", CallingConvention = CallingConvention.StdCall)]
        public static extern void Run2(byte[] program, byte[] arg0, byte[] arg1);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "Run3", CallingConvention = CallingConvention.StdCall)]
        public static extern void Run3(byte[] program, byte[] arg0, byte[] arg1, byte[] arg2);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "Run4", CallingConvention = CallingConvention.StdCall)]
        public static extern void Run4(byte[] program, byte[] arg0, byte[] arg1, byte[] arg2, byte[] arg3);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "NumRun0", CallingConvention = CallingConvention.StdCall)]
        public static extern double NumRun0(byte[] program);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "NumRun1", CallingConvention = CallingConvention.StdCall)]
        public static extern double NumRun1(byte[] program, byte[] arg0);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "NumRun2", CallingConvention = CallingConvention.StdCall)]
        public static extern double NumRun2(byte[] program, byte[] arg0, byte[] arg1);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "NumRun3", CallingConvention = CallingConvention.StdCall)]
        public static extern double NumRun3(byte[] program, byte[] arg0, byte[] arg1, byte[] arg2);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "NumRun4", CallingConvention = CallingConvention.StdCall)]
        public static extern double NumRun4(byte[] program, byte[] arg0, byte[] arg1, byte[] arg2, byte[] arg3);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun0", CallingConvention = CallingConvention.StdCall)]
        public static extern void StrRun0(byte[] program, char[] res);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun1", CallingConvention = CallingConvention.StdCall)]
        public static extern void StrRun1(byte[] program, char[] res, byte[] arg0);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun2", CallingConvention = CallingConvention.StdCall)]
        public static extern void StrRun2(byte[] program, char[] res, byte[] arg0, byte[] arg1);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun3", CallingConvention = CallingConvention.StdCall)]
        public static extern void StrRun3(byte[] program, char[] res, byte[] arg0, byte[] arg1, byte[] arg2);

        [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun4", CallingConvention = CallingConvention.StdCall)]
        public static extern void StrRun4(byte[] program, char[] res, byte[] arg0, byte[] arg1, byte[] arg2, byte[] arg3);

        public static void Run0UDF(SqlBinary byteCode)
        {
            Run0(byteCode.Value);
        }

        public static void Run1UDF(SqlBinary byteCode, SqlBinary arg0)
        {
            Run1(byteCode.Value, arg0.Value);
        }

        public static void Run2UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1)
        {
            Run2(byteCode.Value, arg0.Value, arg1.Value);
        }

        public static void Run3UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2)
        {
            Run3(byteCode.Value, arg0.Value, arg1.Value, arg2.Value);
        }

        public static void Run4UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2, SqlBinary arg3)
        {
            Run4(byteCode.Value, arg0.Value, arg1.Value, arg2.Value, arg3.Value);
        }

        public static SqlDouble NumRun0UDF(SqlBinary byteCode)
        {
            return NumRun0(byteCode.Value);
        }

        public static SqlDouble NumRun1UDF(SqlBinary byteCode, SqlBinary arg0)
        {
            return NumRun1(byteCode.Value, arg0.Value);
        }

        public static SqlDouble NumRun2UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1)
        {
            return NumRun2(byteCode.Value, arg0.Value, arg1.Value);
        }

        public static SqlDouble NumRun3UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2)
        {
            return NumRun3(byteCode.Value, arg0.Value, arg1.Value, arg2.Value);
        }

        public static SqlDouble NumRun4UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2, SqlBinary arg3)
        {
            return NumRun4(byteCode.Value, arg0.Value, arg1.Value, arg2.Value, arg3.Value);
        }

        public static SqlString StrRun0UDF(SqlBinary byteCode)
        {
            char[] res = new char[100];
            StrRun0(byteCode.Value, res);
            return new string(res);
        }

        public static SqlString StrRun1UDF(SqlBinary byteCode, SqlBinary arg0)
        {
            char[] res = new char[100];
            StrRun1(byteCode.Value, res, arg0.Value);
            return new string(res);
        }

        public static SqlString StrRun2UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1)
        {
            char[] res = new char[100];
            StrRun2(byteCode.Value, res, arg0.Value, arg1.Value);
            return new string(res);
        }

        public static SqlString StrRun3UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2)
        {
            char[] res = new char[100];
            StrRun3(byteCode.Value, res, arg0.Value, arg1.Value, arg2.Value);
            return new string(res);
        }

        public static SqlString StrRun4UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2, SqlBinary arg3)
        {
            char[] res = new char[100];
            StrRun4(byteCode.Value, res, arg0.Value, arg1.Value, arg2.Value, arg3.Value);
            return new string(res);
        }
    }
}
