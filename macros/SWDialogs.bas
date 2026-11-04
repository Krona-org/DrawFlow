Attribute VB_Name = "SWDialogs"
Public SWExportFileName As String
Public SWExportFileNameType As OPENFILENAME

#If VBA7 Then
    Private Declare PtrSafe Function GetOpenFileName _
    Lib "COMDLG32.DLL" Alias "GetOpenFileNameA" _
    (pOpenfilename As OPENFILENAME) As LongPtr
    
    Private Declare PtrSafe Function GetSaveFileName _
    Lib "COMDLG32.DLL" Alias "GetSaveFileNameA" _
    (pOpenfilename As OPENFILENAME) As LongPtr
    
    Private Type OPENFILENAME
        lStructSize As Long
        hwndOwner As LongPtr
        hInstance As LongPtr
        lpstrFilter As String
        lpstrCustomFilter As String
        nMaxCustFilter As Long
        nFilterIndex As Long
        lpstrFile As String
        nMaxFile As Long
        lpstrFileTitle As String
        nMaxFileTitle As Long
        lpstrInitialDir As String
        lpstrTitle As String
        Flags As Long
        nFileOffset As Integer
        nFileExtension As Integer
        lpstrDefExt As String
        lCustData As Long
        lpfnHook As LongPtr
        lpTemplateName As String
    End Type
#Else
    Private Declare Function GetOpenFileName _
    Lib "COMDLG32.DLL" Alias "GetOpenFileNameA" _
    (pOpenfilename As OPENFILENAME) As Long
    
    Private Declare Function GetSaveFileName _
    Lib "COMDLG32.DLL" Alias "GetSaveFileNameA" _
    (pOpenfilename As OPENFILENAME) As Long
    
    Private Type OPENFILENAME
        lStructSize As Long
        hwndOwner As Long
        hInstance As Long
        lpstrFilter As String
        lpstrCustomFilter As String
        nMaxCustFilter As Long
        nFilterIndex As Long
        lpstrFile As String
        nMaxFile As Long
        lpstrFileTitle As String
        nMaxFileTitle As Long
        lpstrInitialDir As String
        lpstrTitle As String
        Flags As Long
        nFileOffset As Integer
        nFileExtension As Integer
        lpstrDefExt As String
        lCustData As Long
        lpfnHook As Long
        lpTemplateName As String
    End Type
#End If

Public Function SWGetFileName()
    With SWExportFileNameType
        .lStructSize = LenB(SWExportFileNameType)
        .nMaxFile = 260
        .lpstrFile = VBA.String(.nMaxFile - 1, 0)
        
        If GetSaveFileName(SWExportFileNameType) <> 0 Then
            SWExportFileName = .lpstrFile
            SWGetFileName = 0
        Else
            SWGetFileName = 1
        End If
    End With
End Function
