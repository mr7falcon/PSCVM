using System;
using System.IO;
using System.Text;
using System.Collections.Generic;

namespace Compiler
{
    class Compiler
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
            ARRAY,
            ASTORE,
            AFETCH,
            APUSH,
            DICTIONARY,
            DSTORE,
            DFETCH,
            DINSERT,
            NONE,
            CONCAT,
        };

        public enum VarType : ushort
        {
            STR,
            ARR,
            DICT
        }

        private const ushort c_null = 0x7FF0;

        private static List<byte> byteCode = new List<byte>();

        private static string program;
        private static int i = 0;
        
        private static void ClearSpaces()
        {
            while (program[i] == ' ' || program[i] == '\r' || program[i] == '\t' || program[i] == '\n')
            {
                ++i;
            }
        }

        private static string Split(params char[] separators)
        {
            string word = "";

            if (separators.Length == 0)
            {
                separators = new char[4] { '\r', '\n', '\t', ' '};
            }

            while(true)
            {
                for (int j = 0; j < separators.Length; ++j)
                {
                    if (i == program.Length || program[i] == separators[j])
                    {
                        ++i;
                        return word;
                    }
                }

                word += program[i++];
            }
        }

        private static byte[] AddVar()
        {
            ClearSpaces();

            if (program[i] == '\'' || program[i] == '\"')
            {
                ++i;
                string op = Split('\'', '\"');

                ushort usNull = c_null;
                ushort usType = (ushort)VarType.STR;
                ushort usLength = (ushort)op.Length;
                List<byte> bytes = new List<byte>();
                bytes.AddRange(BitConverter.GetBytes(usNull));
                bytes.AddRange(BitConverter.GetBytes(usType));
                bytes.AddRange(BitConverter.GetBytes(usLength));
                bytes.AddRange(BitConverter.GetBytes((ushort)1));
                bytes.AddRange(Encoding.ASCII.GetBytes(op));
                return bytes.ToArray();
            }
            else if (program[i] == '{')
            {
                ++i;
                ushort usType = (ushort)VarType.ARR;
                List<byte[]> varBytes = new List<byte[]>();
                while (true)
                {
                    varBytes.Add(AddVar());

                    ClearSpaces();
                    char sym = program[i++];
                    if (sym == '}')
                    {
                        break;
                    }
                    else if (sym == ':')
                    {
                        usType = (ushort)VarType.DICT;
                        continue;
                    }
                    else if (sym != '|')
                    {
                        //any exceptopn
                    }
                }

                ushort usNull = c_null;
                ushort usLength = usType == (ushort)VarType.ARR ? (ushort)varBytes.Count : (ushort)(varBytes.Count / 2);
                List<byte> bytes = new List<byte>();
                bytes.AddRange(BitConverter.GetBytes(usNull));
                bytes.AddRange(BitConverter.GetBytes(usType));
                bytes.AddRange(BitConverter.GetBytes(usLength));
                bytes.AddRange(BitConverter.GetBytes((ushort)1));

                for (int j = 0; j < varBytes.Count; ++j)
                {
                    bytes.AddRange(varBytes[j]);
                }

                return bytes.ToArray();
            }
            else
            {
                string op = "";
                while (Char.IsDigit(program[i]) || program[i] == ',' || program[i] == '-')
                {
                    op += program[i++];
                }
                return BitConverter.GetBytes(double.Parse(op));
            }
        }

        private static byte[] AddInt()
        {
            ClearSpaces();
            string op = Split();
            int offset = int.Parse(op);
            return BitConverter.GetBytes(offset);
        }

        private static byte[] AddLong()
        {
            ClearSpaces();
            string op = Split();
            long offset = long.Parse(op);
            return BitConverter.GetBytes(offset);
        }

        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                throw new Exception("expected file name");
            }

            program = File.ReadAllText(args[0]);

            while (i < program.Length)
            {
                ClearSpaces();
                string command = Split();
                switch (command)
                {
                    case "CALL":
                        byteCode.Add((byte)ByteCommand.CALL);
                        byteCode.AddRange(AddLong());
                        break;
                    case "RET":
                        byteCode.Add((byte)ByteCommand.RET);
                        break;
                    case "FETCH":
                        byteCode.Add((byte)ByteCommand.FETCH);
                        byteCode.AddRange(AddInt());
                        break;
                    case "STORE":
                        byteCode.Add((byte)ByteCommand.STORE);
                        AddInt();
                        break;
                    case "LFETCH":
                        byteCode.Add((byte)ByteCommand.LFETCH);
                        byteCode.AddRange(AddInt());
                        break;
                    case "LSTORE":
                        byteCode.Add((byte)ByteCommand.LSTORE);
                        byteCode.AddRange(AddInt());
                        break;
                    case "LALLOC":
                        byteCode.Add((byte)ByteCommand.LALLOC);
                        byteCode.AddRange(AddInt());
                        break;
                    case "LFREE":
                        byteCode.Add((byte)ByteCommand.LFREE);
                        break;
                    case "PUSH":
                        byteCode.Add((byte)ByteCommand.PUSH);
                        byteCode.AddRange(AddVar());
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
                        byteCode.AddRange(AddInt());
                        break;
                    case "DEC":
                        byteCode.Add((byte)ByteCommand.DEC);
                        byteCode.AddRange(AddInt());
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
                        byteCode.AddRange(AddLong());
                        break;
                    case "JNZ":
                        byteCode.Add((byte)ByteCommand.JNZ);
                        byteCode.AddRange(AddLong());
                        break;
                    case "JMP":
                        byteCode.Add((byte)ByteCommand.JMP);
                        byteCode.AddRange(AddLong());
                        break;
                    case "HALT":
                        byteCode.Add((byte)ByteCommand.HALT);
                        break;
                    case "ARR":
                        byteCode.Add((byte)ByteCommand.ARRAY);
                        break;
                    case "DICT":
                        byteCode.Add((byte)ByteCommand.DICTIONARY);
                        break;
                    case "DINS":
                        byteCode.Add((byte)ByteCommand.DINSERT);
                        byteCode.AddRange(AddInt());
                        break;
                    case "AFETCH":
                        byteCode.Add((byte)ByteCommand.AFETCH);
                        byteCode.AddRange(AddInt());
                        break;
                    case "ASTORE":
                        byteCode.Add((byte)ByteCommand.ASTORE);
                        byteCode.AddRange(AddInt());
                        break;
                    case "APUSH":
                        byteCode.Add((byte)ByteCommand.APUSH);
                        byteCode.AddRange(AddInt());
                        break;
                    case "DFETCH":
                        byteCode.Add((byte)ByteCommand.DFETCH);
                        byteCode.AddRange(AddInt());
                        break;
                    case "DSTORE":
                        byteCode.Add((byte)ByteCommand.DSTORE);
                        byteCode.AddRange(AddInt());
                        break;
                    case "CONCAT":
                        byteCode.Add((byte)ByteCommand.CONCAT);
                        break;
                    default:
                        throw new Exception("unknown command");
                }
            }

            using (FileStream fstream = new FileStream("../../../../Tests/Byte/" +
                   Path.GetFileNameWithoutExtension(args[0]) + ".bpsc", FileMode.Create))
            {
                fstream.Write(byteCode.ToArray(), 0, byteCode.Count);
            }
        }
    }
}

