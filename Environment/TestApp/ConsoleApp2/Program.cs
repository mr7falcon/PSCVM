using System.Data.SqlClient;
using System.IO;
using System.Data;
using System.Data.SqlTypes;
using System;
using System.Text;

namespace ConsoleApp4
{
    class Program
    {
        private static string connectionString = @"Data Source=LAPTOP-32R542SA\SQLEXPRESS;Initial Catalog=poso_c;Integrated Security=True";

        static void conn_InfoMessage(object sender, SqlInfoMessageEventArgs e)
        {
            Console.Write(e.Message);
        }

        public static void Func_call()
        {
            using (SqlConnection connection = new SqlConnection(connectionString))
            {
                //SqlBinary code = File.ReadAllBytes(@"C:\Users\mrfal\Documents\VirtualMachine\Environment\Tests\Byte\sarg.bpsc");

                SqlChars codestr = new SqlChars("PUSH -3.21\nNTOS\nPUSH '-3.210000'\nEQ\nASSERT\nHALT".ToCharArray());
                SqlCommand compile = new SqlCommand("CompileUDF", connection);
                compile.CommandType = CommandType.StoredProcedure;
                SqlParameter code = new SqlParameter
                {
                    ParameterName = "@code",
                    Value = codestr,
                    SqlDbType = SqlDbType.NChar
                };
                compile.Parameters.Add(code);

                byte[] bcode = new byte[1024];
                SqlParameter byteCode = new SqlParameter
                {
                    ParameterName = "@byteCode",
                    SqlDbType = SqlDbType.Binary,
                    Direction = ParameterDirection.InputOutput,
                    Value = bcode
                };
                compile.Parameters.Add(byteCode);

                SqlCommand run = new SqlCommand("Run0UDF", connection);
                run.CommandType = CommandType.StoredProcedure;

                //char[] buf = new char[100];
                //SqlParameter output = new SqlParameter
                //{
                //    ParameterName = "@output",
                //    Value = buf,
                //    Direction = ParameterDirection.InputOutput
                //};
                //command.Parameters.Add(output);

                //double num = 5.0;
                //byte[] barg = BitConverter.GetBytes(num);
                //string str = "STRING";
                //byte[] barg = Encoding.ASCII.GetBytes(str);
                //SqlParameter arg = new SqlParameter
                //{
                //    ParameterName = "@arg0",
                //    Value = barg,
                //    SqlDbType = SqlDbType.Binary
                //};
                //run.Parameters.Add(arg);

                connection.Open();

                compile.ExecuteNonQuery();

                SqlParameter program = new SqlParameter
                {
                    ParameterName = "@byteCode",
                    Value = byteCode.Value,
                    SqlDbType = SqlDbType.Binary
                };
                run.Parameters.Add(program);

                run.ExecuteNonQuery();

                connection.Close();
            }
        }

        static void Main(string[] args)
        {
            Func_call();
        }
    }
}