#!./sqlweb -i./jho.ini
<HTML>
Hello World!
<CURSOR SQL="select sysdate as TDATE from dual"><B>:TDate</B>
</CURSOR>
<b>done</b>
</HTML>
