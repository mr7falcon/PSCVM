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
            APOP,
            DERASE,
            PRINT,
            DUP,
            NARG,
            SARG,
            ASSERT,
            XOR,
            NEG,
            SHL,
            SHR,
            BOR,
            BAND,
            LEN,
            DARR,
            DCONT,
            SFETCH,
            SSTORE,
            NTOS,
            STON,
            SMATCH,
            SUBS,
        };

        public enum VarType : ushort
        {
            STR,
            ARR,
            DICT,
            NIL
        }

        private const ushort c_null = 0x7FF0;

        private static List<byte> byteCode = new List<byte>();

        private static string program;
        private static int i = 0;
        
        private struct Coords
        {
            public int oper;
            public int pos;
        }

        private static Dictionary<string, Coords> marks = new Dictionary<string, Coords>();

        internal class CException : Exception
        {
            public CException(string messege)
                : base(byteCode.Count.ToString() + ": " + messege + "\n")
            {}
        }

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

        private static void AddVar(List<byte[]> cont = null)
        {
            byte[] bts;

            ClearSpaces();

            if (Char.IsDigit(program[i]) || program[i] == '-')
            {
                string op = "";
                op += program[i++];
                while (Char.IsDigit(program[i]) || program[i] == '.' || program[i] == ',')
                {
                    op += program[i++];
                }
                op = op.Replace('.', ',');
                bts = BitConverter.GetBytes(double.Parse(op));
            }
            else if (program[i] == '\'' || program[i] == '\"')
            {
                ++i;
                string op = Split('\'', '\"');

                ushort usNull = c_null;
                ushort usType = (ushort)VarType.STR;
                uint usLength = (uint)op.Length;
                List<byte> bytes = new List<byte>();
                bytes.AddRange(BitConverter.GetBytes(usNull));
                bytes.AddRange(BitConverter.GetBytes(usType));
                bytes.AddRange(BitConverter.GetBytes(usLength));
                bytes.AddRange(Encoding.ASCII.GetBytes(op));
                bts = bytes.ToArray();
            }
            else if (program[i] == '{')
            {
                ++i;
                ushort usType = (ushort)VarType.ARR;
                List<byte[]> varBytes = new List<byte[]>();
                while (true)
                {
                    AddVar(varBytes);

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
                        throw new CException("unexpected symbol. Use '|' to separate container members");
                    }
                }

                ushort usNull = c_null;
                uint usLength = usType == (ushort)VarType.ARR ? (uint)varBytes.Count : (uint)(varBytes.Count / 2);
                List<byte> bytes = new List<byte>();
                bytes.AddRange(BitConverter.GetBytes(usNull));
                bytes.AddRange(BitConverter.GetBytes(usType));
                bytes.AddRange(BitConverter.GetBytes(usLength));

                for (int j = 0; j < varBytes.Count; ++j)
                {
                    bytes.AddRange(varBytes[j]);
                }

                bts = bytes.ToArray();
            }
            else
            {
                if (program.Substring(i, 4) == "NULL")
                {
                    i += 4;
                    List<byte> bytes = new List<byte>();
                    bytes.AddRange(BitConverter.GetBytes(c_null));
                    bytes.AddRange(BitConverter.GetBytes((ushort)VarType.NIL));
                    bytes.AddRange(BitConverter.GetBytes((uint)0));
                    bts = bytes.ToArray();
                }
                else
                {
                    throw new CException("can't determine a Variant type");
                }
            }

            if (cont != null)
            {
                cont.Add(bts);
            }
            else
            {
                byteCode.AddRange(bts);
            }
        }

        private static void AddInt()
        {
            ClearSpaces();
            string op = Split();
            int offset = int.Parse(op);
            byteCode.AddRange(BitConverter.GetBytes(offset));
        }

        private static void AddByte(byte b)
        {
            byteCode.Add(b);
        }

        private static void AddMark()
        {
            ClearSpaces();
            string mark = Split();
            Coords coords;
            if (marks.ContainsKey(mark))
            {
                coords = marks[mark];
                coords.pos = byteCode.Count;
            }
            else
            {
                coords = new Coords
                {
                    pos = byteCode.Count
                };
            }
            marks[mark] = coords;
            byteCode.AddRange(BitConverter.GetBytes((long)0));
        }

        private static byte[] Linking()
        {
            byte[] bytes = byteCode.ToArray();
            foreach(KeyValuePair<string, Coords> mark in marks)
            {
                byte[] markBytes = BitConverter.GetBytes((long)mark.Value.oper);
                Array.Copy(markBytes, 0, bytes, mark.Value.pos, 8);
            }
            return bytes;
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
                        AddByte((byte)ByteCommand.CALL);
                        AddMark();
                        break;
                    case "RET":
                        AddByte((byte)ByteCommand.RET);
                        break;
                    case "FETCH":
                        AddByte((byte)ByteCommand.FETCH);
                        AddInt();
                        break;
                    case "STORE":
                        AddByte((byte)ByteCommand.STORE);
                        AddInt();
                        break;
                    case "LFETCH":
                        AddByte((byte)ByteCommand.LFETCH);
                        AddInt();
                        break;
                    case "LSTORE":
                        AddByte((byte)ByteCommand.LSTORE);
                        AddInt();
                        break;
                    case "LALLOC":
                        AddByte((byte)ByteCommand.LALLOC);
                        AddInt();
                        break;
                    case "LFREE":
                        AddByte((byte)ByteCommand.LFREE);
                        break;
                    case "PUSH":
                        AddByte((byte)ByteCommand.PUSH);
                        AddVar();
                        break;
                    case "POP":
                        AddByte((byte)ByteCommand.POP);
                        break;
                    case "ADD":
                        AddByte((byte)ByteCommand.ADD);
                        break;
                    case "SUB":
                        AddByte((byte)ByteCommand.SUB);
                        break;
                    case "MULT":
                        AddByte((byte)ByteCommand.MULT);
                        break;
                    case "DIV":
                        AddByte((byte)ByteCommand.DIV);
                        break;
                    case "INC":
                        AddByte((byte)ByteCommand.INC);
                        AddInt();
                        break;
                    case "DEC":
                        AddByte((byte)ByteCommand.DEC);
                        AddInt();
                        break;
                    case "MOD":
                        AddByte((byte)ByteCommand.MOD);
                        break;
                    case "AND":
                        AddByte((byte)ByteCommand.AND);
                        break;
                    case "OR":
                        AddByte((byte)ByteCommand.OR);
                        break;
                    case "BOR":
                        AddByte((byte)ByteCommand.BOR);
                        break;
                    case "BAND":
                        AddByte((byte)ByteCommand.BAND);
                        break;
                    case "NOT":
                        AddByte((byte)ByteCommand.NOT);
                        break;
                    case "XOR":
                        AddByte((byte)ByteCommand.XOR);
                        break;
                    case "NEG":
                        AddByte((byte)ByteCommand.NEG);
                        AddInt();
                        break;
                    case "SHL":
                        AddByte((byte)ByteCommand.SHL);
                        break;
                    case "SHR":
                        AddByte((byte)ByteCommand.SHR);
                        break;
                    case "LT":
                        AddByte((byte)ByteCommand.LT);
                        break;
                    case "GT":
                        AddByte((byte)ByteCommand.GT);
                        break;
                    case "LET":
                        AddByte((byte)ByteCommand.LET);
                        break;
                    case "GET":
                        AddByte((byte)ByteCommand.GET);
                        break;
                    case "EQ":
                        AddByte((byte)ByteCommand.EQ);
                        break;
                    case "NEQ":
                        AddByte((byte)ByteCommand.NEQ);
                        break;
                    case "JZ":
                        AddByte((byte)ByteCommand.JZ);
                        AddMark();
                        break;
                    case "JNZ":
                        AddByte((byte)ByteCommand.JNZ);
                        AddMark();
                        break;
                    case "JMP":
                        AddByte((byte)ByteCommand.JMP);
                        AddMark();
                        break;
                    case "HALT":
                        AddByte((byte)ByteCommand.HALT);
                        break;
                    case "ARR":
                        AddByte((byte)ByteCommand.ARRAY);
                        break;
                    case "DICT":
                        AddByte((byte)ByteCommand.DICTIONARY);
                        break;
                    case "DINS":
                        AddByte((byte)ByteCommand.DINSERT);
                        AddInt();
                        break;
                    case "AFETCH":
                        AddByte((byte)ByteCommand.AFETCH);
                        AddInt();
                        break;
                    case "ASTORE":
                        AddByte((byte)ByteCommand.ASTORE);
                        AddInt();
                        break;
                    case "APUSH":
                        AddByte((byte)ByteCommand.APUSH);
                        AddInt();
                        break;
                    case "DFETCH":
                        AddByte((byte)ByteCommand.DFETCH);
                        AddInt();
                        break;
                    case "DSTORE":
                        AddByte((byte)ByteCommand.DSTORE);
                        AddInt();
                        break;
                    case "CONCAT":
                        AddByte((byte)ByteCommand.CONCAT);
                        break;
                    case "APOP":
                        AddByte((byte)ByteCommand.APOP);
                        AddInt();
                        break;
                    case "DERASE":
                        AddByte((byte)ByteCommand.DERASE);
                        AddInt();
                        break;
                    case "PRINT":
                        AddByte((byte)ByteCommand.PRINT);
                        break;
                    case "DUP":
                        AddByte((byte)ByteCommand.DUP);
                        break;
                    case "NARG":
                        {
                            AddByte((byte)ByteCommand.NARG);
                            ClearSpaces();
                            string op = Split();
                            AddByte(byte.Parse(op));
                        }
                        break;
                    case "SARG":
                        {
                            AddByte((byte)ByteCommand.SARG);
                            ClearSpaces();
                            string op = Split();
                            AddByte(byte.Parse(op));
                        }
                        break;
                    case "ASSERT":
                        AddByte((byte)ByteCommand.ASSERT);
                        break;
                    case "LEN":
                        AddByte((byte)ByteCommand.LEN);
                        AddInt();
                        break;
                    case "DARR":
                        AddByte((byte)ByteCommand.DARR);
                        AddInt();
                        break;
                    case "DCONT":
                        AddByte((byte)ByteCommand.DCONT);
                        AddInt();
                        break;
                    case "SFETCH":
                        AddByte((byte)ByteCommand.SFETCH);
                        AddInt();
                        break;
                    case "SSTORE":
                        AddByte((byte)ByteCommand.SSTORE);
                        AddInt();
                        break;
                    case "NTOS":
                        AddByte((byte)ByteCommand.NTOS);
                        break;
                    case "STON":
                        AddByte((byte)ByteCommand.STON);
                        break;
                    case "SMATCH":
                        AddByte((byte)ByteCommand.SMATCH);
                        AddInt();
                        break;
                    case "SUBS":
                        AddByte((byte)ByteCommand.SUBS);
                        AddInt();
                        break;
                    default:
                        if (command[command.Length - 1] == ':')
                        {
                            string mark = command.Substring(0, command.Length - 1);
                            Coords coords;
                            if (marks.ContainsKey(mark))
                            {
                                coords = marks[mark];
                                coords.oper = byteCode.Count;
                            }
                            else
                            {
                                coords = new Coords
                                {
                                    oper = byteCode.Count
                                };
                            }
                            marks[mark] = coords;
                        }
                        else
                        {
                            throw new CException("unknown command " + command);
                        }
                        break;
                }
            }

            using (FileStream fstream = new FileStream("../../../../Tests/Byte/" +
                   Path.GetFileNameWithoutExtension(args[0]) + ".bpsc", FileMode.Create))
            {
                fstream.Write(Linking(), 0, byteCode.Count);
            }
        }
    }
}

