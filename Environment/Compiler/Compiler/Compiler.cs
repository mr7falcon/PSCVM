using System;
using System.IO;
using CompilerDll;

namespace Compiler
{
    class Compiler
    {
        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                throw new Exception("expected file name");
            }

            byte[] byteCode = CompilerDll.Compiler.Compile(File.ReadAllText(args[0]));

            using (FileStream fstream = new FileStream("../../../../../Tests/Byte/" +
                   Path.GetFileNameWithoutExtension(args[0]) + ".bpsc", FileMode.Create))
            {
                fstream.Write(byteCode, 0, byteCode.Length);
            }
        }
    }
}

