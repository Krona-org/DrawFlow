Attribute VB_Name = "SWConvert"
Public SWObjectsNum As Integer
Public SWObjects() As Long

Public Function SWCheckMaster(vsoShape As Object)
    SWCheckMaster = 0
    
    If (vsoShape.Master Is Nothing) Then
        SWCheckMaster = 1
        
        Exit Function
    End If

    If Not (StrComp(vsoShape.Master.Name, SWMasterName, vbTextCompare) = 0) Then SWCheckMaster = 1
End Function

Public Function SWDataExport()
    Dim i As Integer
    Dim j As Integer

    Dim TempId As String
    Dim TempAddress As String

    If SWGetFileName = 1 Then
        MsgBox ("Ошибка открытия файла")
        Exit Function
    End If
    
    Dim TmpFileCreation As Integer
    TmpFileCreation = 1
    
    While TmpFileCreation = 1
        Open SWExportFileName For Binary Access Write As #1
        
        If LOF(1) > 0 Then
            If MsgBox("Указанный вами файл не пуст. Перезаписать?", vbYesNo) = vbYes Then
                Close #1
                Kill SWExportFileName
                Open SWExportFileName For Binary Access Write As #1
                TmpFileCreation = 0
            Else
                Close #1
                Exit Function
            End If
        Else
            TmpFileCreation = 0
        End If
    Wend
    
    Dim TmpMicroCom As String
    Dim TmpShape As Visio.Shape
    Dim TmpPos As Integer
        
    SWObjectsNum = 0

    For j = 1 To ActiveDocument.Pages.Count
        For i = 1 To ActiveDocument.Pages(j).Shapes.Count
            If SWCheckMaster(ActiveDocument.Pages(j).Shapes(i)) = 1 Then GoTo lbNextI
            
            Set TmpShape = ActiveDocument.Pages(j).Shapes(i)
            TempId = SWLoad(TmpShape, "IndexID")
            TempAddress = SWLoad(TmpShape, "Address")

            If (Val(TempId) = 0) Or (Val(TempAddress) = 0) Then GoTo lbNextI

            ReDim Preserve SWObjects(3, SWObjectsNum + 1)
            SWObjectsNum = SWObjectsNum + 1
    
            TmpPos = Val(TempAddress) * 10
    
            TmpMicroCom = SWLoad(TmpShape, "Address")
            Put #1, 1 + TmpPos, CInt(CUInt(SWTransNum(VBA.Str(TmpMicroCom), TempAddress)))
            
            TmpMicroCom = SWLoad(TmpShape, "M1")
            Put #1, 3 + TmpPos, CInt(CUInt(SWTransNum(VBA.Str(TmpMicroCom), TempAddress)))
            TmpMicroCom = VBA.Oct(SWTrans8to10(Val(SWLoad(TmpShape, "M6"))) * (2 ^ 11) + SWTrans8to10(Val(SWLoad(TmpShape, "M5"))) * (2 ^ 6) + SWTrans8to10(Val(SWLoad(TmpShape, "M3"))) * (2 ^ 2) + SWTrans8to10(Val(SWLoad(TmpShape, "M2"))))
            Put #1, 5 + TmpPos, CInt(CUInt(SWTransNum(VBA.Str(TmpMicroCom), TempAddress)))
            
            TmpMicroCom = VBA.Oct(SWTrans8to10(Val(SWLoad(TmpShape, "M13"))) * (2 ^ 14) + SWTrans8to10(Val(SWLoad(TmpShape, "M10"))) * (2 ^ 11) + SWTrans8to10(Val(SWLoad(TmpShape, "M9"))) * (2 ^ 8) + SWTrans8to10(Val(SWLoad(TmpShape, "M8"))) * (2 ^ 6) + SWTrans8to10(Val(SWLoad(TmpShape, "M7"))) * (2 ^ 4) + SWTrans8to10(Val(SWLoad(TmpShape, "M4"))))
            Put #1, 7 + TmpPos, CInt(CUInt(SWTransNum(VBA.Str(TmpMicroCom), TempAddress)))
            
            TmpMicroCom = VBA.Oct(SWTrans8to10(Val(SWLoad(TmpShape, "M15"))) * (2 ^ 8) + SWTrans8to10(Val(SWLoad(TmpShape, "M14"))) * (2 ^ 6) + SWTrans8to10(Val(SWLoad(TmpShape, "M12"))) * (2 ^ 3) + SWTrans8to10(Val(SWLoad(TmpShape, "M11"))))
            Put #1, 9 + TmpPos, CInt(CUInt(SWTransNum(VBA.Str(TmpMicroCom), TempAddress)))
lbNextI:
        Next i
    Next j
        
    MsgBox ("Файл данных сформирован. Обработано " & VBA.Str(SWObjectsNum) & " блоков на " & VBA.Str(ActiveDocument.Pages.Count) & " страницах.")
        
    Close #1
End Function

Public Function SWTransNum(vsoStr As String, vsoAddr As String)
    Dim TmpStr As String
    Dim TmpVal As Long
    Dim TmpStrLen As Integer
    Dim TmpNullStr As String
    Dim SWBitNum As Integer
    
    vsoStr = Replace(vsoStr, " ", "")
    TmpStr = SWTrans8to2(vsoStr)
    TmpStrLen = Len(TmpStr)
    
    SWBitNum = 16
    If TmpStrLen < 1 Or TmpStrLen > SWBitNum Then
        MsgBox ("Ошибка преобразования данных в микрокоманде " & vsoAddr & ". Некоторые данные могут быть некорректно сохранены.")
        
        Exit Function
    End If
    
    Dim i As Integer
    For i = 1 To SWBitNum - TmpStrLen
        TmpNullStr = "0" & TmpNullStr
    Next i
    
    TmpStr = Replace(TmpNullStr + TmpStr, " ", "")

    SWTransNum = Val(SWTrans2to10(TmpStr))
End Function
