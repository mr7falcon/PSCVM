using System;
using System.IO;
using System.Text;
using System.Collections.Generic;

namespace Compiler
{
    unsafe class Compiler
    {
        public enum ByteCommand : byte
        {
            CALL,
            RET,
            FETCH,
            STORE,
            LFETCH,
            LSTORE,
            LALLOC,
            LFREE,
            PUSH,
            POP,
            ADD,
            SUB,
            INC,
            DEC,
            MULT,
            DIV,
            MOD,
            AND,
            OR,
            NOT,
            LT,
            GT,
            LET,
            GET,
            EQ,
            NEQ,
            JZ,
            JNZ,
            JMP,
            HALT,

            NONE
        };

        public enum VarType : ushort
        {
            STR,
            ARR
        }

        private const ushort c_null = 0x7FF0;

        private static List<byte> byteCode = new List<byte>();

        private static void AddVar(string op)
        {
            if (op[0] == '\'')
            {
                if (op[op.Length - 1] != '\'')
                {
                    throw new Exception("string ending error");
                }

                string sValue = op.Substring(1, op.Length - 1).Substring(0, op.Length - 2);
                ushort usNull = c_null;
                ushort usType = (ushort)VarType.STR;
                int nLength = sValue.Length;
                byteCode.AddRange(BitConverter.GetBytes(usNull));
                byteCode.AddRange(BitConverter.GetBytes(usType));
                byteCode.AddRange(BitConverter.GetBytes(nLength));
                byteCode.AddRange(Encoding.ASCII.GetBytes(sValue));
            }
            else if (op[0] == '{')
            {
                if (op[op.Length - 1] != '}')
                {
                    throw new Exception("list ending error");
                }

                string[] strVars = op.Substring(1, op.Length - 1).Substring(0, op.Length - 2).Split('|');

                for (int i = 0; i < strVars.Length; ++i)
                {
                    AddVar(strVars[i]);
                }
            }
            else
            {
                byteCode.AddRange(BitConverter.GetBytes(double.Parse(op)));
            }
        }

        private static void AddInt(string op)
        {
            int offset = int.Parse(op);
            byteCode.AddRange(BitConverter.GetBytes(offset));
        }

        private static void AddLong(string op)
        {
            long offset = long.Parse(op);
            byteCode.AddRange(BitConverter.GetBytes(offset));
        }

        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                throw new Exception("expected file name");
            }

            string[] separators = { " ", "\n", "\r", "\t" };
            string[] words = File.ReadAllText(args[0]).Split(separators, StringSplitOptions.RemoveEmptyEntries);

            for (int i = 0; i < words.Length;)
            {
                switch (words[i++])
                {
                    case "CALL":
                        byteCode.Add((byte)ByteCommand.CALL);
                        AddLong(words[i++]);
                        break;
                    case "RET":
                        byteCode.Add((byte)ByteCommand.RET);
                        break;
                    case "FETCH":
                        byteCode.Add((byte)ByteCommand.FETCH);
                        AddInt(words[i++]);
                        break;
                    case "STORE":
                        byteCode.Add((byte)ByteCommand.STORE);
                        AddInt(words[i++]);
                        break;
                    case "LFETCH":
                        byteCode.Add((byte)ByteCommand.LFETCH);
                        AddInt(words[i++]);
                        break;
                    case "LSTORE":
                        byteCode.Add((byte)ByteCommand.LSTORE);
                        AddInt(words[i++]);
                        break;
                    case "LALLOC":
                        byteCode.Add((byte)ByteCommand.LALLOC);
                        AddInt(words[i++]);
                        break;
                    case "LFREE":
                        byteCode.Add((byte)ByteCommand.LFREE);
                        break;
                    case "PUSH":
                        byteCode.Add((byte)ByteCommand.PUSH);
                        AddVar(words[i++]);
                        break;
                    case "POP":
                        byteCode.Add((byte)ByteCommand.POP);
                        break;
                    case "ADD":
                        byteCode.Add((byte)ByteCommand.ADD);
                        break;
                    case "SUB":
                        byteCode.Add((byte)ByteCommand.SUB);
                        break;
                    case "MULT":
                        byteCode.Add((byte)ByteCommand.MULT);
                        break;
                    case "DIV":
                        byteCode.Add((byte)ByteCommand.DIV);
                        break;
                    case "INC":
                        byteCode.Add((byte)ByteCommand.INC);
                        break;
                    case "DEC":
                        byteCode.Add((byte)ByteCommand.DEC);
                        break;
                    case "MOD":
                        byteCode.Add((byte)ByteCommand.MOD);
                        break;
                    case "AND":
                        byteCode.Add((byte)ByteCommand.AND);
                        break;
                    case "OR":
                        byteCode.Add((byte)ByteCommand.OR);
                        break;
                    case "NOT":
                        byteCode.Add((byte)ByteCommand.NOT);
                        break;
                    case "LT":
                        byteCode.Add((byte)ByteCommand.LT);
                        break;
                    case "GT":
                        byteCode.Add((byte)ByteCommand.GT);
                        break;
                    case "LET":
                        byteCode.Add((byte)ByteCommand.LET);
                        break;
                    case "GET":
                        byteCode.Add((byte)ByteCommand.GET);
                        break;
                    case "EQ":
                        byteCode.Add((byte)ByteCommand.EQ);
                        break;
                    case "NEQ":
                        byteCode.Add((byte)ByteCommand.NEQ);
                        break;
                    case "JZ":
                        byteCode.Add((byte)ByteCommand.JZ);
                        AddLong(words[i++]);
                        break;
                    case "JNZ":
                        byteCode.Add((byte)ByteCommand.JNZ);
                        AddLong(words[i++]);
                        break;
                    case "JMP":
                        byteCode.Add((byte)ByteCommand.JMP);
                        AddLong(words[i++]);
                        break;
                    case "HALT":
                        byteCode.Add((byte)ByteCommand.HALT);
                        break;
                    default:
                        throw new Exception("unknown command");
                }
            }

            using (FileStream fstream = new FileStream("../../../../Tests/" +
                   Path.GetFileNameWithoutExtension(args[0]) + ".bpsc", FileMode.Create))
            {
                fstream.Write(byteCode.ToArray(), 0, byteCode.Count);
            }
        }
    }
}

