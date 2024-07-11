#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define KiloBytes(Count) (Count * 1024)
#define MegaBytes(Count) (KiloBytes(Count) * 1024)

typedef int32_t i32;
typedef uint32_t u32;
typedef uint8_t byte;

enum command_flags
{
    Command_Error = (1 << 0),
};

struct command_result
{
    u32 Flags;
    i32 ExitCode;
    u32 BytesRead;
};

struct buffer
{
    byte *Base;
    u32 Offset;
    u32 Size;
    u32 Capacity;
};

struct string
{
    char *Base;
    u32 Size;
};

struct arena
{
    byte *Base;
    u32 Offset;
    u32 Capacity;
};

byte *PushBytes(arena *Arena, u32 Size)
{
    u32 Align = 8;
    u32 Start = (Arena->Offset + Align - 1) & ~((u32) (Align - 1));
    assert(Start + Size <= Arena->Capacity);
    Arena->Offset = Start + Size;
    return &Arena->Base[Start];
}

string PushString(arena *Arena, const char *Base, u32 Size)
{
    string Result = {};
    Result.Base = (char *) PushBytes(Arena, Size);
    Result.Size = Size;
    memcpy(Result.Base, Base, Size);
    return Result;
}

u32 ReadFile(const char *Path, byte *Buffer, u32 BufferSize)
{
    u32 Result = 0;
    FILE *File = {};
    if (fopen_s(&File, Path, "rb") == 0) 
    {
        Result = fread(Buffer, 1, BufferSize, File);
        fclose(File);
    } 
    else 
    {
        fprintf(stderr, "Failed to open file \"%s\"\n", Path);
    }

    return Result;
}

command_result ExecuteCommand(const char *Command, byte *Buffer, u32 BufferSize)
{
    command_result Result = {};
    // Use `popen` and `pclose` with gcc (and prob clang) respectivly 
    FILE *Process = _popen(Command, "r");
    if (Process)
    {
        Result.BytesRead = fread(Buffer, 1, BufferSize, Process);
        Result.ExitCode = _pclose(Process);
    }
    else
    {
        fprintf(stderr, "Failed to execute command  \"%s\"\n", Command);
        Result.Flags |= Command_Error;
    }
    
    return Result;
}

bool IsLetter(char Char)
{
    return (Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z');
}

bool IsDigit(char Char)
{
    return Char >= 0 && Char <= 9;
}

char Peek(buffer *Buffer)
{
    return Buffer->Base[Buffer->Offset];
}

string ReadIdent(buffer *Buffer, arena *Arena)
{
    char Current = Peek(Buffer);
    u32 Size = 0;
    char *Start = (char*) &Buffer->Base[Buffer->Offset];
    while (IsLetter(Current) || IsDigit(Current)) {
        Buffer->Offset++;
        Size++;
        Current = Peek(Buffer);
    }

    return PushString(Arena, Start, Size);
}

i32 main()
{
    buffer Buffer = {};
    Buffer.Capacity = MegaBytes(1);
    Buffer.Base = (byte *) malloc(Buffer.Capacity);
    Buffer.Size = ReadFile("builds.txt", Buffer.Base, Buffer.Capacity);

    arena Arena = {};
    Arena.Capacity = MegaBytes(1);
    Arena.Base = (byte *) malloc(Arena.Capacity);

    while (Buffer.Offset < Buffer.Size)
    {
        char Current = Peek(&Buffer);

        if (IsLetter(Current)) 
        {
            string Ident = ReadIdent(&Buffer, &Arena);
            for (u32 I = 0; I < Ident.Size; ++I) {
                printf("%c", Ident.Base[I]);
            }
            printf("%u", Ident.Size);
            break;
        }
    }

    // byte OutputBuffer[1024] = {};
    // command_result CommandResult = ExecuteCommand(Buffer, OutputBuffer, sizeof(OutputBuffer));
    //
    // if (CommandResult.ExitCode) 
    // {
    //     fprintf(stdout, "%s\n", OutputBuffer);
    // }
    // else 
    // {
    //     fprintf(stdout, "Ok.");
    // }

    return 0;
}
