Attribute VB_Name = "SWGlobal"
Public Function SWMasterName()
    SWMasterName = "K1804 MasterBlock"
End Function

Public Function SWCurrentObject()
    #If VBA7 And Win64 Then
        Set SWCurrentObject = ActiveWindow.Selection.ContainingShape
    #Else
        Set SWCurrentObject = ActiveWindow.Selection(1)
    #End If
End Function

Public Function SWCurrentShape()
    SWCurrentShape = CInt(ActiveWindow.Selection(1).ID)
End Function

Public Function SWTrans10to8(Num As Integer)
    SWTrans10to8 = VBA.Oct(Num)
End Function

Public Function SWTrans8to10(Num As Integer)
    Dim i As Integer
    Dim TmpNum As Integer
    Dim TmpSign As Integer
    Dim TmpNum3 As Integer
    
    TmpNum = 0
    TmpSign = 0
    TmpNum3 = Num
    
    For i = 0 To Len(VBA.Str(Num)) - 2
        TmpSign = TmpNum3 \ (10 ^ (Len(VBA.Str(Num)) - 2 - i))
        TmpNum = TmpNum + TmpSign * (8 ^ (Len(VBA.Str(Num)) - 2 - i))
        TmpNum3 = TmpNum3 - TmpSign * (10 ^ (Len(VBA.Str(Num)) - 2 - i))
    Next i

    SWTrans8to10 = TmpNum
End Function

Public Function SWTrans2to10(Num As String)
    Dim TmpNum As String
    Dim TmpRes As Long
    Dim i As Integer
    
    TmpRes = 0
    
    For i = 1 To Len(Num)
        TmpRes = TmpRes + Val(VBA.Mid(Num, Len(Num) + 1 - i, 1)) * (2 ^ (i - 1))
    Next i
    
    SWTrans2to10 = TmpRes
End Function

Public Function SWTrans8to2(Num As String)
    Dim i As Integer
    Dim j As Integer
    Dim TmpNum As Long
    Dim TmpStr As String
    Dim TmpChar As String
    Dim TmpResStr As String
    Dim Tmp1 As String
    
    For i = 1 To Len(Num)
        TmpNum = Val(VBA.Mid(Num, Len(Num) + 1 - i, 1))
        TmpStr = ""
        
        For j = 1 To 3
            TmpChar = VBA.Str(Fix(TmpNum / 2 ^ (3 - j)))
            TmpStr = TmpStr + TmpChar
            TmpStr = Replace(TmpStr, " ", "")
            
            If (StrComp(TmpChar, " 1", vbTextCompare) = 0) Then
                TmpNum = TmpNum - 2 ^ (3 - j)
            End If
        Next j
        
        TmpResStr = TmpStr + TmpResStr
    Next i
    
    Dim TmpFlag As Integer
    Dim TmpPos As Integer
    TmpFlag = 0
    
    TmpPos = InStr(1, TmpResStr, "1", 1)
        
    If (TmpPos) > 1 Then
        TmpResStr = VBA.Mid(TmpResStr, TmpPos)
    End If
            
    SWTrans8to2 = TmpResStr
End Function

Public Function CUInt(Num As Long)
    If (Num > 2 ^ 15 - 1) Then
        CUInt = CInt(-(2 ^ 16 - Num))
    Else
        CUInt = CInt(Num)
    End If
End Function
