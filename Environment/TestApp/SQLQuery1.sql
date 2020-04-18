USE poso_c
GO

ALTER DATABASE poso_c SET TRUSTWORTHY ON

CREATE ASSEMBLY UDP FROM 'C:\Users\mrfal\Documents\VirtualMachine\SqlProc\SqlProc\bin\Release\SqlProc.dll' with permission_set=UNSAFE;
GO

CREATE PROCEDURE Run0UDF @byteCode binary(1024) AS EXTERNAL NAME UDP.StoredProcedures.Run0UDF;
GO

CREATE PROCEDURE Run1UDF @byteCode binary(1024), @arg0 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.Run1UDF;
GO

CREATE PROCEDURE Run2UDF @byteCode binary(1024), @arg0 binary(100), @arg1 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.Run2UDF;
GO

CREATE PROCEDURE Run3UDF @byteCode binary(1024), @arg0 binary(100), @arg1 binary(100), @arg2 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.Run3UDF;
GO

CREATE PROCEDURE Run4UDF @byteCode binary(1024), @arg0 binary(100), @arg1 binary(100), @arg2 binary(100), @arg3 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.Run4UDF;
GO

CREATE PROCEDURE NumRun0UDF @byteCode binary(1024), @output float OUTPUT AS EXTERNAL NAME UDP.StoredProcedures.NumRun0UDF;
GO

CREATE PROCEDURE NumRun1UDF @byteCode binary(1024), @output float OUTPUT, @arg0 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.NumRun1UDF;
GO

CREATE PROCEDURE NumRun2UDF @byteCode binary(1024), @output float OUTPUT, @arg0 binary(100), @arg1 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.NumRun2UDF;
GO

CREATE PROCEDURE NumRun3UDF @byteCode binary(1024), @output float OUTPUT, @arg0 binary(100), @arg1 binary(100), @arg2 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.NumRun3UDF;
GO

CREATE PROCEDURE NumRun4UDF @byteCode binary(1024), @output float OUTPUT, @arg0 binary(100), @arg1 binary(100), @arg2 binary(100), @arg3 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.NumRun4UDF;
GO

CREATE PROCEDURE StrRun0UDF @byteCode binary(1024), @output nchar(100) OUTPUT AS EXTERNAL NAME UDP.StoredProcedures.StrRun0UDF;
GO

CREATE PROCEDURE StrRun1UDF @byteCode binary(1024), @output nchar(100) OUTPUT, @arg0 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.StrRun1UDF;
GO

CREATE PROCEDURE StrRun2UDF @byteCode binary(1024), @output nchar(100) OUTPUT, @arg0 binary(100), @arg1 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.StrRun2UDF;
GO

CREATE PROCEDURE StrRun3UDF @byteCode binary(1024), @output nchar(100) OUTPUT, @arg0 binary(100), @arg1 binary(100), @arg2 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.StrRun3UDF;
GO

CREATE PROCEDURE StrRun4UDF @byteCode binary(1024), @output nchar(100) OUTPUT, @arg0 binary(100), @arg1 binary(100), @arg2 binary(100), @arg3 binary(100) AS EXTERNAL NAME UDP.StoredProcedures.StrRun4UDF;
GO

CREATE ASSEMBLY COMP FROM 'C:\Users\mrfal\Documents\VirtualMachine\Environment\Compiler\SqlCompiler\SqlCompiler\bin\Release\SqlCompiler.dll' with permission_set=UNSAFE;
GO

CREATE PROCEDURE CompileUDF @code nchar(1024), @byteCode binary(1024) OUTPUT AS EXTERNAL NAME COMP.StoredProcedures.CompileUDF;
GO