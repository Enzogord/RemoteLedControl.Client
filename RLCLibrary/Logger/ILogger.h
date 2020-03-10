#pragma once

class ILogger
{
public:
    virtual bool IsEnabled() = 0;

    virtual void Enable() = 0;
    virtual void Disable() = 0;

    virtual void PrintNewLine() = 0;
    virtual void Print(const char message[], bool newLine = true) = 0;
    virtual void Print(const char message[], const char content[], bool newLine = true) = 0;
    virtual void Print(const char message[], char content, bool newLine = true) = 0;
    virtual void Print(const char message[], unsigned char content, bool newLine = true) = 0;
    virtual void Print(const char message[], int content, bool newLine = true) = 0;
    virtual void Print(const char message[], unsigned int content, bool newLine = true) = 0;
    virtual void Print(const char message[], long content, bool newLine = true) = 0;
    virtual void Print(const char message[], unsigned long content, bool newLine = true) = 0;
    virtual void Print(const char message[], double content, bool newLine = true) = 0;
};