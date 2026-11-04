Attribute VB_Name = "SWModData2"
Public SWVarM5 As Integer
Public SWVarM6 As Integer
Public SWArrayM7(4, 2) As String
Public SWArrayM7Count As Integer
Public SWArrayM8(4, 2) As String
Public SWArrayM8Count As Integer
Public SWArrayM9(8, 2) As String
Public SWArrayM9Count As Integer
Public SWArrayM10(8, 2) As String
Public SWArrayM10Count As Integer
Public SWArrayM11(8, 6) As String
Public SWArrayM11Count As Integer

Public Sub SWVarM5Init()
    SWVarM5 = 0
End Sub

Public Sub SWVarM6Init()
    SWVarM6 = 0
End Sub

Public Function SWCorrectM5M6(Num As String)
    If (Len(Num) < 6) Then
        SWCorrectM5M6 = "000" + Num
    Else
        SWCorrectM5M6 = Num
    End If
End Function

Public Sub SWArrayM7Init()
    SWArrayM7(0, 1) = "'0' --> [R] --> [PSW(0)]  ;  '0' --> [RQ] -->"
    SWArrayM7(1, 1) = "==> [R] --> [PSW(0)] ==> (цикл)  ;  ==> [RQ] ==> (цикл)"
    SWArrayM7(2, 1) = "[PSW(0)] --> [R] --> [RQ] --> [PSW(0)]"
    SWArrayM7(3, 1) = "[(F15[+]OVR)] --> [R]--> [RQ] --> [PSW(0)]"
    SWArrayM7Count = 3
End Sub

Public Sub SWArrayM7InitAlt()
    SWArrayM7(0, 1) = "[PSW(0)] <-- [R] <-- '0'  ;  <-- [RQ] <-- '0'"
    SWArrayM7(1, 1) = "<== [PSW(0)] <-- [R] <== (цикл)  ;  <-- [RQ] <-- (цикл)"
    SWArrayM7(2, 1) = "[PSW(0)] <-- [R] <-- [RQ] <-- '1'"
    SWArrayM7(3, 1) = "[PSW(0)] <-- [R] <-- [RQ] <-- '0'"
    SWArrayM7Count = 3
End Sub

Public Sub SWArrayM8Init()
    SWArrayM8(0, 1) = "'0'"
    SWArrayM8(1, 1) = "'1'"
    SWArrayM8(2, 1) = "PSW[0]"
    SWArrayM8(3, 1) = "NOT PSW[0]"
    SWArrayM8Count = 3
End Sub

Public Sub SWArrayM9Init()
    SWArrayM9(0, 1) = "A :: Q"
    SWArrayM9(1, 1) = "A :: B"
    SWArrayM9(2, 1) = "0 :: Q"
    SWArrayM9(3, 1) = "0 :: B"
    SWArrayM9(4, 1) = "0 :: A"
    SWArrayM9(5, 1) = "D :: A"
    SWArrayM9(6, 1) = "D :: Q"
    SWArrayM9(7, 1) = "D :: 0"
    SWArrayM9Count = 7
End Sub

Public Sub SWArrayM10Init()
    SWArrayM10(0, 1) = "R + S + C0"
    SWArrayM10(1, 1) = "S - R - 1 + C0"
    SWArrayM10(2, 1) = "R - S - 1 + C0"
    SWArrayM10(3, 1) = "R v S"
    SWArrayM10(4, 1) = "R & S"
    SWArrayM10(5, 1) = "NOT(R) & S"
    SWArrayM10(6, 1) = "R [+] S"
    SWArrayM10(7, 1) = "NOT (R [+] S)"
    SWArrayM10Count = 7
End Sub

Public Sub SWArrayM11Init()
    SWArrayM11(0, 1) = " - "
    SWArrayM11(1, 1) = " - "
    SWArrayM11(2, 1) = " - "
    SWArrayM11(3, 1) = " - "
    SWArrayM11(4, 1) = "-->"
    SWArrayM11(5, 1) = "-->"
    SWArrayM11(6, 1) = "<--"
    SWArrayM11(7, 1) = "<--"
    
    SWArrayM11(0, 2) = "    -   "
    SWArrayM11(1, 2) = "    -   "
    SWArrayM11(2, 2) = "  F -> B"
    SWArrayM11(3, 2) = "  F -> B"
    SWArrayM11(4, 2) = "F/2 -> B"
    SWArrayM11(5, 2) = "F/2 -> B"
    SWArrayM11(6, 2) = "2*F -> B"
    SWArrayM11(7, 2) = "2*F -> B"
    
    SWArrayM11(0, 3) = " - "
    SWArrayM11(1, 3) = " - "
    SWArrayM11(2, 3) = " - "
    SWArrayM11(3, 3) = " - "
    SWArrayM11(4, 3) = "-->"
    SWArrayM11(5, 3) = " - "
    SWArrayM11(6, 3) = "<--"
    SWArrayM11(7, 3) = " - "
    
    SWArrayM11(0, 4) = "  F -> Q"
    SWArrayM11(1, 4) = "    -   "
    SWArrayM11(2, 4) = "    -   "
    SWArrayM11(3, 4) = "    -   "
    SWArrayM11(4, 4) = "Q/2 -> Q"
    SWArrayM11(5, 4) = "    -   "
    SWArrayM11(6, 4) = "2*Q -> Q"
    SWArrayM11(7, 4) = "    -   "
    
    SWArrayM11(0, 5) = "F"
    SWArrayM11(1, 5) = "F"
    SWArrayM11(2, 5) = "A"
    SWArrayM11(3, 5) = "F"
    SWArrayM11(4, 5) = "F"
    SWArrayM11(5, 5) = "F"
    SWArrayM11(6, 5) = "F"
    SWArrayM11(7, 5) = "F"
    
    SWArrayM11Count = 7
End Sub
