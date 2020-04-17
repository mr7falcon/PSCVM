using System.Data.SqlTypes;
using System.Runtime.InteropServices;
using System.Text;

public partial class StoredProcedures
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
    public static extern int StrRun0(byte[] program, StringBuilder res);

    [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun1", CallingConvention = CallingConvention.StdCall)]
    public static extern int StrRun1(byte[] program, StringBuilder res, byte[] arg0);

    [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun2", CallingConvention = CallingConvention.StdCall)]
    public static extern int StrRun2(byte[] program, StringBuilder res, byte[] arg0, byte[] arg1);

    [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun3", CallingConvention = CallingConvention.StdCall)]
    public static extern int StrRun3(byte[] program, StringBuilder res, byte[] arg0, byte[] arg1, byte[] arg2);

    [DllImport("VirtualMachine_x64.dll", EntryPoint = "StrRun4", CallingConvention = CallingConvention.StdCall)]
    public static extern int StrRun4(byte[] program, StringBuilder res, byte[] arg0, byte[] arg1, byte[] arg2, byte[] arg3);

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void Run0UDF(SqlBinary byteCode)
    {
        Run0(byteCode.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void Run1UDF(SqlBinary byteCode, SqlBinary arg0)
    {
        Run1(byteCode.Value, arg0.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void Run2UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1)
    {
        Run2(byteCode.Value, arg0.Value, arg1.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void Run3UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2)
    {
        Run3(byteCode.Value, arg0.Value, arg1.Value, arg2.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void Run4UDF(SqlBinary byteCode, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2, SqlBinary arg3)
    {
        Run4(byteCode.Value, arg0.Value, arg1.Value, arg2.Value, arg3.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void NumRun0UDF(SqlBinary byteCode, ref SqlDouble output)
    {
       output = NumRun0(byteCode.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void NumRun1UDF(SqlBinary byteCode, ref SqlDouble output, SqlBinary arg0)
    {
        output = NumRun1(byteCode.Value, arg0.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void NumRun2UDF(SqlBinary byteCode, ref SqlDouble output, SqlBinary arg0, SqlBinary arg1)
    {
        output = NumRun2(byteCode.Value, arg0.Value, arg1.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void NumRun3UDF(SqlBinary byteCode, ref SqlDouble output, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2)
    {
        output = NumRun3(byteCode.Value, arg0.Value, arg1.Value, arg2.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void NumRun4UDF(SqlBinary byteCode, ref SqlDouble output, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2, SqlBinary arg3)
    {
        output = NumRun4(byteCode.Value, arg0.Value, arg1.Value, arg2.Value, arg3.Value);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void StrRun0UDF(SqlBinary byteCode, ref SqlChars output)
    {
        StringBuilder buf = new StringBuilder(100);
        int len = StrRun0(byteCode.Value, buf);
        output.Write(0, buf.ToString().ToCharArray(), 0, len);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void StrRun1UDF(SqlBinary byteCode, ref SqlChars output, SqlBinary arg0)
    {
        StringBuilder buf = new StringBuilder(100);
        int len = StrRun1(byteCode.Value, buf, arg0.Value);
        output.Write(0, buf.ToString().ToCharArray(), 0, len);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void StrRun2UDF(SqlBinary byteCode, ref SqlChars output, SqlBinary arg0, SqlBinary arg1)
    {
        StringBuilder buf = new StringBuilder(100);
        int len = StrRun2(byteCode.Value, buf, arg0.Value, arg1.Value);
        output.Write(0, buf.ToString().ToCharArray(), 0, len);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void StrRun3UDF(SqlBinary byteCode, ref SqlChars output, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2)
    {
        StringBuilder buf = new StringBuilder(100);
        int len = StrRun3(byteCode.Value, buf, arg0.Value, arg1.Value, arg2.Value);
        output.Write(0, buf.ToString().ToCharArray(), 0, len);
    }

    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void StrRun4UDF(SqlBinary byteCode, ref SqlChars output, SqlBinary arg0, SqlBinary arg1, SqlBinary arg2, SqlBinary arg3)
    {
        StringBuilder buf = new StringBuilder(100);
        int len = StrRun4(byteCode.Value, buf, arg0.Value, arg1.Value, arg2.Value, arg3.Value);
        output.Write(0, buf.ToString().ToCharArray(), 0, len);
    }
}

