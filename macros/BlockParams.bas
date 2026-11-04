Attribute VB_Name = "BlockParams"
Attribute VB_Base = "0{B9E7C118-2381-4D90-BE74-DC6287C386E5}{901B336C-14EE-468E-B079-D0C3205C6557}"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Attribute VB_TemplateDerived = False
Attribute VB_Customizable = False
Public Function SWShapeText(VisObj As Object)
    Dim TmpStr As String
    Dim TmpStr2 As String
    TmpStr = ""
        
    VisObj.Text = ""
    VisObj.Shapes(2).Text = TextBox4.Text
    VisObj.Shapes(3).Text = TextBox5.Text
    
    TmpStr = TmpStr & ComboBox1.Text & vbCrLf
    
    Dim TmpStr2A As String, TmpStr2B As String, TmpStr2R As String, TmpStr2S As String
    
    If (OptionButton2.Value = True) Then
        TmpStr2A = "RK[2/0]"
    ElseIf (OptionButton3.Value = True) Then
        TmpStr2A = "RK[8/6]"
    Else
        TmpStr2A = "R" + VBA.Str(SWTrans2to10(TextBox8.Value + TextBox9.Value + TextBox10.Value + TextBox11.Value + TextBox12.Value))
    End If
    
    If (OptionButton5.Value = True) Then
        TmpStr2B = "RK[2/0]"
    ElseIf (OptionButton6.Value = True) Then
        TmpStr2B = "RK[8/6]"
    Else
        TmpStr2B = "R" + VBA.Str(SWTrans2to10(TextBox23.Value + TextBox24.Value + TextBox25.Value + TextBox26.Value + TextBox27.Value))
    End If
    
    Select Case ComboBox9.ListIndex
    Case 0:
        TmpStr2R = TmpStr2A
        TmpStr2S = "Q"
    Case 1:
        TmpStr2R = TmpStr2A
        TmpStr2S = TmpStr2B
    Case 2:
        TmpStr2R = "0"
        TmpStr2S = "Q"
    Case 3:
        TmpStr2R = "0"
        TmpStr2S = TmpStr2B
    Case 4:
        TmpStr2R = "0"
        TmpStr2S = TmpStr2A
    Case 5:
        TmpStr2R = "D"
        TmpStr2S = TmpStr2A
    Case 6:
        TmpStr2R = "D"
        TmpStr2S = "Q"
    Case 7:
        TmpStr2R = "D"
        TmpStr2S = "0"
    End Select
    
    If (ComboBox11.ListIndex = 0) Then
        TmpStr2 = "NOP"
        GoTo TmpStr2Label
    End If

    If (ComboBox11.ListIndex = 1) Then
            Select Case ComboBox10.ListIndex
    Case 0:
        TmpStr2 = TmpStr2 + TmpStr2R + " + " + TmpStr2S
    Case 1:
        TmpStr2 = TmpStr2 + TmpStr2S + " - " + TmpStr2R + " - 1"
    Case 2:
        TmpStr2 = TmpStr2 + TmpStr2R + " - " + TmpStr2S + " - 1"
    Case 3:
        TmpStr2 = TmpStr2 + TmpStr2R + " V " + TmpStr2S
    Case 4:
        TmpStr2 = TmpStr2 + TmpStr2R + " & " + TmpStr2S
    Case 5:
        TmpStr2 = TmpStr2 + "NOT(" + TmpStr2R + ") & " + TmpStr2S
    Case 6:
        TmpStr2 = TmpStr2 + TmpStr2R + " [+] " + TmpStr2S
    Case 7:
        TmpStr2 = TmpStr2 + "NOT(" + TmpStr2R + " [+] " + TmpStr2S + ")"
    End Select
        GoTo TmpStr2Label
    End If

    If (ComboBox11.ListIndex = 5 Or ComboBox11.ListIndex = 4) Then
    Select Case ComboBox10.ListIndex
    Case 0:
        TmpStr2 = TmpStr2B + " <--( " + TmpStr2R + " + " + TmpStr2S + ")/2"
    Case 1:
        TmpStr2 = TmpStr2B + " <--( " + TmpStr2S + " - " + TmpStr2R + " - 1/2)"
    Case 2:
        TmpStr2 = TmpStr2B + " <--( " + TmpStr2R + " - " + TmpStr2S + " - 1)/2"
    Case 3:
        TmpStr2 = TmpStr2B + " <--( " + TmpStr2R + " V " + TmpStr2S + ")/2"
    Case 4:
        TmpStr2 = TmpStr2B + " <--( " + TmpStr2R + " & " + TmpStr2S + ")/2"
    Case 5:
        TmpStr2 = TmpStr2B + " <--( " + "NOT(" + TmpStr2R + ") & " + TmpStr2S + ")/2"
    Case 6:
        TmpStr2 = TmpStr2B + " <--( " + TmpStr2R + " [+] " + TmpStr2S + ")/2"
    Case 7:
        TmpStr2 = TmpStr2B + " <--( " + "NOT(" + TmpStr2R + " [+] " + TmpStr2S + ")" + ")/2"
    End Select
        GoTo TmpStr2Label
    End If

    If (ComboBox11.ListIndex = 7 Or ComboBox11.ListIndex = 6) Then
    Select Case ComboBox10.ListIndex
    Case 0:
        TmpStr2 = TmpStr2B + " <--2*( " + TmpStr2R + " + " + TmpStr2S + ")"
    Case 1:
        TmpStr2 = TmpStr2B + " <--2*( " + TmpStr2S + " - " + TmpStr2R + " - 1)"
    Case 2:
        TmpStr2 = TmpStr2B + " <--2*( " + TmpStr2R + " - " + TmpStr2S + " - 1)"
    Case 3:
        TmpStr2 = TmpStr2B + " <--2*( " + TmpStr2R + " V " + TmpStr2S + ")"
    Case 4:
        TmpStr2 = TmpStr2B + " <--2*( " + TmpStr2R + " & " + TmpStr2S + ")"
    Case 5:
        TmpStr2 = TmpStr2B + " <--2*( " + "NOT(" + TmpStr2R + ") & " + TmpStr2S + ")"
    Case 6:
        TmpStr2 = TmpStr2B + " <--2*( " + TmpStr2R + " [+] " + TmpStr2S + ")"
    Case 7:
        TmpStr2 = TmpStr2B + " <--2*( " + "NOT(" + TmpStr2R + " [+] " + TmpStr2S + ")" + ")"
    End Select
        GoTo TmpStr2Label
    End If

    Select Case ComboBox10.ListIndex
    Case 0:
        TmpStr2 = TmpStr2B + " <-- " + TmpStr2R + " + " + TmpStr2S
    Case 1:
        TmpStr2 = TmpStr2B + " <-- " + TmpStr2S + " - " + TmpStr2R + " - 1"
    Case 2:
        TmpStr2 = TmpStr2B + " <-- " + TmpStr2R + " - " + TmpStr2S + " - 1"
    Case 3:
        TmpStr2 = TmpStr2B + " <-- " + TmpStr2R + " V " + TmpStr2S
    Case 4:
        TmpStr2 = TmpStr2B + " <-- " + TmpStr2R + " & " + TmpStr2S
    Case 5:
        TmpStr2 = TmpStr2B + " <-- " + "NOT(" + TmpStr2R + ") & " + TmpStr2S
    Case 6:
        TmpStr2 = TmpStr2B + " <-- " + TmpStr2R + " [+] " + TmpStr2S
    Case 7:
        TmpStr2 = TmpStr2B + " <-- " + "NOT(" + TmpStr2R + " [+] " + TmpStr2S + ")"
    End Select
    
TmpStr2Label:
    
    TmpStr = TmpStr & TmpStr2 & vbCrLf
    
    Select Case ComboBox4.ListIndex
    Case 0:
        TmpStr = TmpStr & "PSW <-- PSW" & vbCrLf
    Case 1:
        TmpStr = TmpStr & "PSW <-- C16, OVR, Z, F15" & vbCrLf
    Case 2:
        TmpStr = TmpStr & "PSW <-- OVR, Z, F15 [+] OVR" & vbCrLf
    Case 3:
        TmpStr = TmpStr & "PSW <-- Y" & vbCrLf
    End Select
    
    If (StrComp(ComboBox2.Text, "null", vbTextCompare) = 0) Then
        TmpStr = TmpStr & "NOP" & vbCrLf
    Else
        TmpStr = TmpStr & ComboBox2.Text & vbCrLf
    End If
    
    TmpStr = TmpStr & SWArrayM4Short(ComboBox5.ListIndex) & "; TST = "
    If (ComboBox3.ListIndex = 1) Or (ComboBox3.ListIndex = 3) Then
        TmpStr = TmpStr & "NOT "
    End If
    TmpStr = TmpStr & ComboBox6.Text & vbCrLf
    
    If Not (IsNumeric(Val(TextBox6.Text))) Then
        TmpStr = TmpStr & "M1: " & "0" & "; "
    Else
        TmpStr = TmpStr & "M1: " & TextBox6.Text & "; "
    End If
    If Not (IsNumeric(Val(TextBox7.Text))) Then
        TmpStr = TmpStr & "M15: " & "0"
    Else
        TmpStr = TmpStr & "M15: " & TextBox7.Text
    End If
    
    VisObj.Shapes(1).Text = TmpStr
End Function

Private Sub CommandButton2_Click()
    On Error GoTo CommandButton2_Click_Err

    If SWFormCheck = 1 Then Exit Sub
    
    Call SWSave(SWCurrentObject, "Name", TextBox4.Text)
    Call SWSave(SWCurrentObject, "Address", TextBox5.Text)
    Call SWSave(SWCurrentObject, "M1", VBA.Str(TextBox6.Text))
    Call SWSave(SWCurrentObject, "M12", ComboBox1.ListIndex)
    Call SWSave(SWCurrentObject, "M13", ComboBox4.ListIndex)
    Call SWSave(SWCurrentObject, "M2", ComboBox3.ListIndex)
    Call SWSave(SWCurrentObject, "M3", SWTrans10to8(ComboBox6.ListIndex))
    Call SWSave(SWCurrentObject, "M4", SWTrans10to8(ComboBox5.ListIndex))

    Select Case SWVarM5
        Case 1
            Call SWSave(SWCurrentObject, "M5", SWTrans10to8(SWTrans2to10(TextBox8.Value + TextBox9.Value + TextBox10.Value + TextBox11.Value + TextBox12.Value)))
        Case 2
            Call SWSave(SWCurrentObject, "M5", SWTrans10to8(SWTrans2to10(TextBox14.Value + TextBox13.Value + TextBox16.Value + TextBox17.Value + TextBox15.Value)))
        Case 3
            Call SWSave(SWCurrentObject, "M5", SWTrans10to8(SWTrans2to10(TextBox18.Value + TextBox20.Value + TextBox21.Value + TextBox22.Value + TextBox19.Value)))
    End Select
        
    Select Case SWVarM6
        Case 1
            Call SWSave(SWCurrentObject, "M6", SWTrans10to8(SWTrans2to10(TextBox23.Value + TextBox24.Value + TextBox25.Value + TextBox26.Value + TextBox27.Value)))
        Case 2
            Call SWSave(SWCurrentObject, "M6", SWTrans10to8(SWTrans2to10(TextBox29.Value + TextBox28.Value + TextBox31.Value + TextBox32.Value + TextBox30.Value)))
        Case 3
            Call SWSave(SWCurrentObject, "M6", SWTrans10to8(SWTrans2to10(TextBox33.Value + TextBox35.Value + TextBox36.Value + TextBox37.Value + TextBox34.Value)))
    End Select
        
    Call SWSave(SWCurrentObject, "M14", ComboBox2.ListIndex)
    Call SWSave(SWCurrentObject, "M7", ComboBox7.ListIndex)
    Call SWSave(SWCurrentObject, "M8", ComboBox8.ListIndex)
    Call SWSave(SWCurrentObject, "M9", ComboBox9.ListIndex)
    Call SWSave(SWCurrentObject, "M10", ComboBox10.ListIndex)
    Call SWSave(SWCurrentObject, "M11", ComboBox11.ListIndex)
    Call SWSave(SWCurrentObject, "M15", TextBox7.Text)
    Call SWShapeText(SWCurrentObject)

    Unload Me

    Exit Sub

CommandButton2_Click_Err:
    MsgBox ("Ошибка. Невозможно выполнить экспорт данных блока.")
    
End Sub

Public Function SWFormCheck()
    Dim i As Integer
    Dim j As Integer
    Dim CurrentName As String
    Dim CurrentAddress As String
    Dim TempName As String
    Dim TempAddress As String
    
    SWFormCheck = 0
    
    CurrentName = TextBox4.Text
    CurrentAddress = TextBox5.Text

    If (Val(CurrentName) < 0 Or Val(CurrentAddress) < 0) Then
        MsgBox ("Некорректные значения в полях ИМЯ и АДРЕС")
        SWFormCheck = 1
        Exit Function
    End If

    For j = 1 To ActiveDocument.Pages.Count
        For i = 1 To ActiveDocument.Pages(j).Shapes.Count
            If SWCheckMaster(ActiveDocument.Pages(j).Shapes(i)) = 1 Then GoTo lbNextI
            
            If ((ActiveDocument.Pages(j) = ActivePage) And (ActiveDocument.Pages(j).Shapes(i) = SWCurrentObject)) Then GoTo lbNextI

            TempName = SWLoad(ActiveDocument.Pages(j).Shapes(i), "Name")
            TempAddress = SWLoad(ActiveDocument.Pages(j).Shapes(i), "Address")
            
            If (Val(TempName) = 0) Or (Val(TempAddress) = 0) Then GoTo lbNextI
            
            If (StrComp(TempName, CurrentName, vbTextCompare) = 0) Then
                MsgBox ("Значение поля АДРЕС БЛОКА данного блока конфликтует со значением этого поля другого блока")
                SWFormCheck = 1
                Exit Function
            End If
            
            If (StrComp(TempAddress, CurrentAddress, vbTextCompare) = 0) Then
                MsgBox ("Значение поля АДРЕС МИКРОКОМАНДЫ данного блока конфликтует со значением этого поля другого блока")
                SWFormCheck = 1
                Exit Function
            End If
        
lbNextI:
        Next i
    Next j
    
End Function

Private Sub OptionButton1_Click()
    SWVarM5 = 1
End Sub

Private Sub OptionButton2_Click()
    SWVarM5 = 2
End Sub
Private Sub OptionButton3_Click()
    SWVarM5 = 3
End Sub
Private Sub OptionButton4_Click()
    SWVarM6 = 1
End Sub
Private Sub OptionButton5_Click()
    SWVarM6 = 2
End Sub
Private Sub OptionButton6_Click()
    SWVarM6 = 3
End Sub

Private Sub UserForm_Initialize()
    'BlockParams.Caption = "Параметры блока" + VBA.Str(SWCurrentObject.ID)
End Sub

Private Sub UserForm_Activate()
    Call SWFormActionM12
    Call SWFormActionM13
    Call SWFormActionM14
    Call SWFormActionM1
    Call SWFormActionM4
    Call SWFormActionM2
    Call SWFormActionM3
    Call SWFormActionM5
    Call SWFormActionM6
    Call SWFormActionM7
    Call SWFormActionM8
    Call SWFormActionM9
    Call SWFormActionM10
    Call SWFormActionM11
    Call SWFormActionMain
End Sub

Public Function SWFormActionMain()
    TextBox4.Text = SWLoad(SWCurrentObject, "Name")
    TextBox5.Text = SWLoad(SWCurrentObject, "Address")
End Function

Public Function SWFormActionM5()
    Dim TmpStr As String
    Dim i As Integer
    
    TmpStr = SWLoad(SWCurrentObject, "M5")
    Select Case Val(TmpStr)
        Case 0 To 1
            OptionButton1.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            TextBox8.Value = 0
            TextBox9.Value = 0
            TextBox10.Value = 0
            TextBox11.Value = 0
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox12.Value = 1
        Case 2 To 3
            OptionButton1.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            TextBox8.Value = 0
            TextBox9.Value = 0
            TextBox10.Value = 0
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox11.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox12.Value = 1
        Case 4 To 7
            OptionButton1.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            TextBox8.Value = 0
            TextBox9.Value = 0
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox10.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox11.Value = 1
            If (VBA.Mid(TmpStr, 6, 1) = 1) Then TextBox12.Value = 1
        Case 10 To 17
            OptionButton1.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            If (VBA.Mid(TmpStr, 3, 1) = 1) Then TextBox8.Value = 1
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox9.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox10.Value = 1
            If (VBA.Mid(TmpStr, 6, 1) = 1) Then TextBox11.Value = 1
            If (VBA.Mid(TmpStr, 7, 1) = 1) Then TextBox12.Value = 1
        Case 18 To 27
            OptionButton2.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox14.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox13.Value = 1
            If (VBA.Mid(TmpStr, 6, 1) = 1) Then TextBox16.Value = 1
            If (VBA.Mid(TmpStr, 7, 1) = 1) Then TextBox17.Value = 1
            If (VBA.Mid(TmpStr, 8, 1) = 1) Then TextBox15.Value = 1
        Case 28 To 37
            OptionButton3.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox18.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox20.Value = 1
            If (VBA.Mid(TmpStr, 6, 1) = 1) Then TextBox21.Value = 1
            If (VBA.Mid(TmpStr, 7, 1) = 1) Then TextBox22.Value = 1
            If (VBA.Mid(TmpStr, 8, 1) = 1) Then TextBox19.Value = 1
        Case Else
            MsgBox ("Ошибка! Значение в ячейке M5 некорректно.")
    End Select
End Function

Public Function SWFormActionM6()
    Dim TmpStr As String
    Dim i As Integer
    
    TmpStr = SWLoad(SWCurrentObject, "M6")
    Select Case Val(TmpStr)
        Case 0 To 1
            OptionButton4.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            TextBox23.Value = 0
            TextBox24.Value = 0
            TextBox25.Value = 0
            TextBox26.Value = 0
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox27.Value = 1
       Case 2 To 3
            OptionButton4.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            TextBox23.Value = 0
            TextBox24.Value = 0
            TextBox25.Value = 0
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox26.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox27.Value = 1
       Case 4 To 7
            OptionButton4.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            TextBox23.Value = 0
            TextBox24.Value = 0
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox25.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox26.Value = 1
            If (VBA.Mid(TmpStr, 6, 1) = 1) Then TextBox27.Value = 1
       Case 10 To 17
            OptionButton4.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            If (VBA.Mid(TmpStr, 3, 1) = 1) Then TextBox23.Value = 1
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox24.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox25.Value = 1
            If (VBA.Mid(TmpStr, 6, 1) = 1) Then TextBox26.Value = 1
            If (VBA.Mid(TmpStr, 7, 1) = 1) Then TextBox27.Value = 1
        Case 18 To 27
            OptionButton5.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox29.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox28.Value = 1
            If (VBA.Mid(TmpStr, 6, 1) = 1) Then TextBox31.Value = 1
            If (VBA.Mid(TmpStr, 7, 1) = 1) Then TextBox32.Value = 1
            If (VBA.Mid(TmpStr, 8, 1) = 1) Then TextBox30.Value = 1
        Case 28 To 37
            OptionButton6.Value = True
            TmpStr = SWCorrectM5M6(SWTrans8to2(TmpStr))
            If (VBA.Mid(TmpStr, 4, 1) = 1) Then TextBox33.Value = 1
            If (VBA.Mid(TmpStr, 5, 1) = 1) Then TextBox35.Value = 1
            If (VBA.Mid(TmpStr, 6, 1) = 1) Then TextBox36.Value = 1
            If (VBA.Mid(TmpStr, 7, 1) = 1) Then TextBox37.Value = 1
            If (VBA.Mid(TmpStr, 8, 1) = 1) Then TextBox34.Value = 1
        Case Else
            MsgBox ("Ошибка! Значение в ячейке M6 некорректно.")
    End Select
    
End Function

Public Function SWFormActionM7()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox7.ColumnCount = 2
    
    For i = 0 To SWArrayM7Count
        SWArrayM7(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox7.List() = SWArrayM7
    TmpStr = SWLoad(SWCurrentObject, "M7")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox7.ListIndex = 0
    Else
        ComboBox7.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function

Public Function SWFormActionM8()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox8.ColumnCount = 2
    
    For i = 0 To SWArrayM8Count
        SWArrayM8(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox8.List() = SWArrayM8
    TmpStr = SWLoad(SWCurrentObject, "M8")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox8.ListIndex = 0
    Else
        ComboBox8.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function

Public Function SWFormActionM9()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox9.ColumnCount = 2
    
    For i = 0 To SWArrayM9Count
        SWArrayM9(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox9.List() = SWArrayM9
    TmpStr = SWLoad(SWCurrentObject, "M9")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox9.ListIndex = 0
    Else
        ComboBox9.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function

Public Function SWFormActionM10()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox10.ColumnCount = 2
    
    For i = 0 To SWArrayM10Count
        SWArrayM10(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox10.List() = SWArrayM10
    TmpStr = SWLoad(SWCurrentObject, "M10")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox10.ListIndex = 0
    Else
        ComboBox10.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function

Public Function SWFormActionM11()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox11.ColumnCount = 6
    
    For i = 0 To SWArrayM11Count
        SWArrayM11(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox11.List() = SWArrayM11
    TmpStr = SWLoad(SWCurrentObject, "M11")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox11.ListIndex = 0
    Else
        ComboBox11.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function

Public Function SWFormActionM14()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox2.ColumnCount = 2
    
    For i = 0 To SWArrayM14Count
        SWArrayM14(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox2.List() = SWArrayM14
    TmpStr = SWLoad(SWCurrentObject, "M14")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox2.ListIndex = 0
    Else
        ComboBox2.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function
Public Function SWFormActionM13()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox4.ColumnCount = 2
    
    For i = 0 To SWArrayM13Count
        SWArrayM13(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox4.List() = SWArrayM13
    TmpStr = SWLoad(SWCurrentObject, "M13")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox4.ListIndex = 0
    Else
        ComboBox4.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function

Public Function SWFormActionM4()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox5.ColumnCount = 2
    
    For i = 0 To SWArrayM4Count
        SWArrayM4(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox5.List() = SWArrayM4
    TmpStr = SWLoad(SWCurrentObject, "M4")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox5.ListIndex = 0
    Else
        ComboBox5.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function

Public Function SWFormActionM3()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox6.ColumnCount = 2
    
    For i = 0 To SWArrayM3Count
        SWArrayM3(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox6.List() = SWArrayM3
    TmpStr = SWLoad(SWCurrentObject, "M3")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox6.ListIndex = 0
    Else
        ComboBox6.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function
Public Function SWFormActionM2()
    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox3.ColumnCount = 2
    
    For i = 0 To SWArrayM2Count
        SWArrayM2(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox3.List() = SWArrayM2
    TmpStr = SWLoad(SWCurrentObject, "M2")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox3.ListIndex = 0
    Else
        ComboBox3.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function

Public Function SWFormActionM1()
    TextBox6.Text = SWLoad(SWCurrentObject, "M1")
    TextBox7.Text = SWLoad(SWCurrentObject, "M15")
End Function

Public Function SWFormActionM12()
    TextBox1.Text = SWLoad(SWCurrentObject, "M1")

    Dim i As Integer
    Dim TmpStr As String
    
    ComboBox1.ColumnCount = 2
    
    For i = 0 To SWArrayM12Count
        SWArrayM12(i, 0) = SWTrans10to8(i)
    Next i
    
    ComboBox1.List() = SWArrayM12
    TmpStr = SWLoad(SWCurrentObject, "M12")
    
    If StrComp(TmpStr, "", vbTextCompare) = 0 Then
        ComboBox1.ListIndex = 0
    Else
        ComboBox1.ListIndex = SWTrans8to10(Val(TmpStr))
    End If
End Function

Private Sub TextBox4_Change()
    If IsNumeric(TextBox4.Text) = False Then
        MsgBox ("Ошибка! Введено некорректное значение в поле адреса блока")
        TextBox4.Text = "0"
    End If
End Sub

Private Sub TextBox5_Change()
    If IsNumeric(TextBox5.Text) = False Then
        MsgBox ("Ошибка! Введено некорректное значение в поле адреса микрокоманды." & vbCrLf & "Поле должно содержать число в восьмиричном формате.")
        TextBox5.Text = "0"
    End If
End Sub

Private Sub ComboBox1_Change()
    TextBox1.Locked = True
    TextBox1.BackColor = &H8000000F
    
    If (ComboBox1.ListIndex > 7) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M12")
        ComboBox1.ListIndex = 0
    End If
End Sub

Private Sub TextBox1_Change()
    If IsNumeric(TextBox1.Text) = False Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M1")
        TextBox1.Text = "0"
    End If
End Sub

Private Sub ComboBox2_Change()
    If (ComboBox2.ListIndex > 3) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M14")
        ComboBox2.ListIndex = 0
    End If
End Sub

Private Sub ComboBox5_Change()
    If (ComboBox5.ListIndex > 15) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M4")
        ComboBox5.ListIndex = 0
    End If
End Sub

Private Sub ComboBox6_Change()
    If (ComboBox6.ListIndex > 15) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M3")
        ComboBox6.ListIndex = 0
    End If
End Sub

Private Sub ComboBox7_Change()
    If (ComboBox7.ListIndex > 3) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M7")
        ComboBox7.ListIndex = 0
    End If
End Sub

Private Sub ComboBox8_Change()
    If (ComboBox8.ListIndex > 3) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M8")
        ComboBox8.ListIndex = 0
    End If
End Sub

Private Sub ComboBox9_Change()
    If (ComboBox9.ListIndex > 7) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M9")
        ComboBox9.ListIndex = 0
    End If
End Sub

Private Sub ComboBox10_Change()
    If (ComboBox10.ListIndex > 7) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M10")
        ComboBox10.ListIndex = 0
    End If
End Sub

Private Sub ComboBox11_Change()
    If (ComboBox11.ListIndex > 7) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M11")
        ComboBox11.ListIndex = 0
    End If
    
    If (ComboBox11.ListIndex = 1) Or (ComboBox11.ListIndex = 3) Or (ComboBox11.ListIndex = 5) Or (ComboBox11.ListIndex = 7) Then
        Call SWArrayM7InitAlt
        Call SWFormActionM7
    Else
        Call SWArrayM7Init
        Call SWFormActionM7
    End If
End Sub

Private Sub ComboBox3_Change()
    If (ComboBox3.ListIndex > 3) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M2")
        ComboBox3.ListIndex = 0
    End If
    
    Dim OldIndex As Integer
    OldIndex = ComboBox6.ListIndex
    
    If ((ComboBox3.ListIndex = 2) Or (ComboBox3.ListIndex = 3)) Then
        Call SWArrayM3InitAlt
        Call SWFormActionM3
    Else
        If ((ComboBox3.ListIndex = 0) Or (ComboBox3.ListIndex = 1)) Then
            Call SWArrayM3Init
            Call SWFormActionM3
        End If
    End If
    
    ComboBox6.ListIndex = OldIndex
End Sub

Private Sub ComboBox4_Change()
    If (ComboBox4.ListIndex > 3) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле M13")
        ComboBox4.ListIndex = 0
    End If
    
    Label13.Caption = SWArrayM13Help(ComboBox4.ListIndex)
End Sub

Private Sub TextBox6_Change()
    If IsNumeric(TextBox6.Text) = False Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M1." & vbCrLf & "Поле должно содержать число в восьмиричном формате.")
        TextBox6.Text = "0"
    End If
End Sub

Private Sub TextBox7_Change()
    If IsNumeric(TextBox7.Text) = False Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M15." & vbCrLf & "Поле должно содержать число в восьмиричном формате.")
        TextBox7.Text = "0"
    End If
End Sub

Private Sub TextBox21_Change()
    If ((IsNumeric(TextBox21.Text) = False) Or (Val(TextBox21.Text) < 0) Or (Val(TextBox21.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox21.Text = "0"
    End If
    
    OptionButton3.Value = True
End Sub

Private Sub TextBox22_Change()
    If ((IsNumeric(TextBox22.Text) = False) Or (Val(TextBox22.Text) < 0) Or (Val(TextBox22.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox22.Text = "0"
    End If
    
    OptionButton3.Value = True
End Sub

Private Sub TextBox19_Change()
    If ((IsNumeric(TextBox19.Text) = False) Or (Val(TextBox19.Text) < 0) Or (Val(TextBox19.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox19.Text = "0"
    End If
    OptionButton3.Value = True
End Sub

Private Sub TextBox16_Change()
    If ((IsNumeric(TextBox16.Text) = False) Or (Val(TextBox16.Text) < 0) Or (Val(TextBox16.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox16.Text = "0"
    End If
    
    OptionButton2.Value = True
End Sub

Private Sub TextBox17_Change()
    If ((IsNumeric(TextBox17.Text) = False) Or (Val(TextBox17.Text) < 0) Or (Val(TextBox17.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox17.Text = "0"
    End If
    
    OptionButton2.Value = True
End Sub

Private Sub TextBox15_Change()
    If ((IsNumeric(TextBox15.Text) = False) Or (Val(TextBox15.Text) < 0) Or (Val(TextBox15.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox15.Text = "0"
    End If
    
    OptionButton2.Value = True
End Sub

Private Sub TextBox11_Change()
    If ((IsNumeric(TextBox11.Text) = False) Or (Val(TextBox11.Text) < 0) Or (Val(TextBox11.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox11.Text = "0"
    End If
    
    OptionButton1.Value = True
End Sub

Private Sub TextBox12_Change()
    If ((IsNumeric(TextBox12.Text) = False) Or (Val(TextBox12.Text) < 0) Or (Val(TextBox12.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox12.Text = "0"
    End If
    
    OptionButton1.Value = True
End Sub

Private Sub TextBox10_Change()
    If ((IsNumeric(TextBox10.Text) = False) Or (Val(TextBox10.Text) < 0) Or (Val(TextBox10.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox10.Text = "0"
    End If
    
    OptionButton1.Value = True
End Sub

Private Sub TextBox9_Change()
    If ((IsNumeric(TextBox9.Text) = False) Or (Val(TextBox9.Text) < 0) Or (Val(TextBox9.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox9.Text = "0"
    End If
    
    OptionButton1.Value = True
End Sub

Private Sub TextBox36_Change()
    If ((IsNumeric(TextBox36.Text) = False) Or (Val(TextBox36.Text) < 0) Or (Val(TextBox36.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox36.Text = "0"
    End If
    
    OptionButton6.Value = True
End Sub

Private Sub TextBox37_Change()
    If ((IsNumeric(TextBox37.Text) = False) Or (Val(TextBox37.Text) < 0) Or (Val(TextBox37.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox37.Text = "0"
    End If
    
    OptionButton6.Value = True
End Sub

Private Sub TextBox34_Change()
    If ((IsNumeric(TextBox34.Text) = False) Or (Val(TextBox34.Text) < 0) Or (Val(TextBox34.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox34.Text = "0"
    End If
    
    OptionButton6.Value = True
End Sub

Private Sub TextBox31_Change()
    If ((IsNumeric(TextBox31.Text) = False) Or (Val(TextBox31.Text) < 0) Or (Val(TextBox31.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox31.Text = "0"
    End If
    
    OptionButton5.Value = True
End Sub

Private Sub TextBox32_Change()
    If ((IsNumeric(TextBox32.Text) = False) Or (Val(TextBox32.Text) < 0) Or (Val(TextBox32.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox32.Text = "0"
    End If
    
    OptionButton5.Value = True
End Sub

Private Sub TextBox30_Change()
    If ((IsNumeric(TextBox30.Text) = False) Or (Val(TextBox30.Text) < 0) Or (Val(TextBox30.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox30.Text = "0"
    End If
    
    OptionButton5.Value = True
End Sub

Private Sub TextBox26_Change()
    If ((IsNumeric(TextBox26.Text) = False) Or (Val(TextBox26.Text) < 0) Or (Val(TextBox26.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox26.Text = "0"
    End If
    
    OptionButton4.Value = True
End Sub

Private Sub TextBox27_Change()
    If ((IsNumeric(TextBox27.Text) = False) Or (Val(TextBox27.Text) < 0) Or (Val(TextBox27.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox27.Text = "0"
    End If
    
    OptionButton4.Value = True
End Sub

Private Sub TextBox25_Change()
    If ((IsNumeric(TextBox25.Text) = False) Or (Val(TextBox25.Text) < 0) Or (Val(TextBox25.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox25.Text = "0"
    End If
    
    OptionButton4.Value = True
End Sub

Private Sub TextBox24_Change()
    If ((IsNumeric(TextBox24.Text) = False) Or (Val(TextBox24.Text) < 0) Or (Val(TextBox24.Text) > 1)) Then
        MsgBox ("Ошибка! Введено некорректное значение в поле констант M5." & vbCrLf & "Биты поля должны иметь значение 0 или 1.")
        TextBox24.Text = "0"
    End If
    
    OptionButton4.Value = True
End Sub

Private Sub ComboBox1_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox1.SetFocus
    ComboBox1.DropDown
End Sub

Private Sub ComboBox2_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox2.SetFocus
    ComboBox2.DropDown
End Sub

Private Sub ComboBox3_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox3.SetFocus
    ComboBox3.DropDown
End Sub

Private Sub ComboBox4_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox4.SetFocus
    ComboBox4.DropDown
End Sub

Private Sub ComboBox5_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox5.SetFocus
    ComboBox5.DropDown
End Sub

Private Sub ComboBox6_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox6.SetFocus
    ComboBox6.DropDown
End Sub

Private Sub ComboBox7_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox7.SetFocus
    ComboBox7.DropDown
End Sub

Private Sub ComboBox8_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox8.SetFocus
    ComboBox8.DropDown
End Sub

Private Sub ComboBox9_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox9.SetFocus
    ComboBox9.DropDown
End Sub

Private Sub ComboBox10_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox10.SetFocus
    ComboBox10.DropDown
End Sub

Private Sub ComboBox11_MouseDown(ByVal Button As Integer, ByVal Shift As Integer, ByVal X As Single, ByVal Y As Single)
    ComboBox11.SetFocus
    ComboBox11.DropDown
End Sub

Private Sub CommandButton1_Click()
    Unload Me
End Sub
