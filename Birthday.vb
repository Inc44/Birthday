' vbnc -optimize Birthday.vb > /dev/null && mono ./Birthday.exe
Imports System
Imports System.Diagnostics
Imports System.Threading
Module Birthday
Const DAYS_IN_YEAR As UInt16 = 365
Const NUM_THREADS As UInt16 = 768
Const PEOPLE As Byte = 24
Const TOTAL_SIMULATIONS As UInt32 = 1000000
Const MULTIPLIER As UInt32 = 1664525
Const INCREMENT As UInt32 = 1013904223
Class ThreadData
Public Simulations As UInt32
Public ThreadId As UInt16
Public SuccessCount As UInt32()
Sub Simulate()
	Dim simulationsPerThread As UInt32 = Simulations \ NUM_THREADS
	Dim seed As UInt64 = Stopwatch.GetTimestamp()
	Dim state As UInt64 = (seed Xor ThreadId) And &HFFFFFFFFUL
	Dim localSuccessCount As UInt32 = 0
	Dim birthdays(DAYS_IN_YEAR - 1) As Byte
	Dim sim As UInt32
	Dim i As UInt16
	For sim = 0 To simulationsPerThread - 1
	Array.Clear(birthdays, 0, DAYS_IN_YEAR)
	For i = 0 To PEOPLE - 1
		state = (state * MULTIPLIER + INCREMENT) And &HFFFFFFFFUL
		Dim birthday As UInt16 = state Mod DAYS_IN_YEAR
		birthdays(birthday) = birthdays(birthday) + 1
	Next
	Dim exactlyTwoCount As Byte = 0
	For i = 0 To DAYS_IN_YEAR - 1
		If birthdays(i) = 2 Then
			exactlyTwoCount += 1
		End If
	Next
	If exactlyTwoCount = 1 Then
		localSuccessCount += 1
	End If
Next
SuccessCount(ThreadId) = localSuccessCount
End Sub
End Class
Sub Main()
	Dim startTime As Stopwatch = Stopwatch.StartNew()
	Dim successCount(NUM_THREADS - 1) As UInt32
	Dim threads(NUM_THREADS - 1) As Thread
	Dim thread(NUM_THREADS - 1) As ThreadData
	For t As UInt16 = 0 To NUM_THREADS - 1
		thread(t) = New ThreadData()
		thread(t).Simulations = TOTAL_SIMULATIONS
		thread(t).ThreadId = t
		thread(t).SuccessCount = successCount
		threads(t) = New Thread(AddressOf thread(t).Simulate)
		threads(t).Start()
	Next
	For t As UInt16 = 0 To NUM_THREADS - 1
		threads(t).Join()
	Next
	Dim totalSuccessCount As UInt32 = 0
	For t As UInt16 = 0 To NUM_THREADS - 1
		totalSuccessCount += successCount(t)
	Next
	Dim probability As Double = totalSuccessCount / TOTAL_SIMULATIONS
	Console.WriteLine("Probability: {0:F9}", probability)
	Dim elapsedTime As Double = startTime.Elapsed.TotalSeconds
	Console.WriteLine("Execution Time: {0:F3} s", elapsedTime)
End Sub
End Module