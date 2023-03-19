#ifndef _COMMANDER_H_INCLUDED
#define _COMMANDER_H_INCLUDED
class cCommander
   {public:
    bool            m_showProgressB;
    private:
    HANDLE          m_hPipeParentRcv;
    HANDLE          m_hPipeParentRcvError;
    HANDLE          m_hPipeChildRcv;
	char            m_errorsCs[256];
    bool            m_processing, m_compiling, m_withinAstep, m_cloning;
    char            m_configuration[80];

    public:
    cCommander(const char *argP);
    ~cCommander();
    int             ShowNtError(const char *fncP, int line);
    int             SendCmd    (const char *cmdP);
    static DWORD WINAPI RcvFromChildThread     (LPVOID lpvThreadParam);
    static DWORD WINAPI RcvErrorFromChildThread(LPVOID lpvThreadParam);
    DWORD WINAPI    Receive     (HANDLE hPipe);
    void            LaunchChild (HANDLE hChildStdOut, HANDLE hChildStdIn, HANDLE hChildStdErr);
   }; //cCommander...

#endif// _COMMANDER_H_INCLUDED...
 