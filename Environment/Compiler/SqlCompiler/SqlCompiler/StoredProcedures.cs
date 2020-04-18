using System.Data.SqlTypes;
using CompilerDll;

public partial class StoredProcedures
{
    [Microsoft.SqlServer.Server.SqlProcedure]
    public static void CompileUDF(SqlChars code, ref SqlBytes byteCode)
    {
        byte[] res = CompilerDll.Compiler.Compile(new string(code.Value));
        byteCode.Write(0, res, 0, res.Length);
    }
}