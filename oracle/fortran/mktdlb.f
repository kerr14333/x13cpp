      SUBROUTINE mktdlb(Tdstr,Ntdchr,Itdtst,Aicstk,Aicrgm,Tdzero,Sp)
      IMPLICIT NONE
c     ------------------------------------------------------------------
c     Generate string for label of trading day effect within AIC test
c     for trading day
c-----------------------------------------------------------------------
      INCLUDE 'notset.prm'
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      CHARACTER tdstr*(30),datstr*(10)
      INTEGER Itdtst,Aicrgm,Aicstk,Ntdchr,nchdat,Tdzero,Sp
      DIMENSION Aicrgm(2)
c-----------------------------------------------------------------------
      CALL setchr(' ',30,tdstr)
      IF(Itdtst.eq.1)THEN
       ntdchr=2
       tdstr(1:ntdchr)='td'
      ELSE IF(Itdtst.eq.2)THEN
       ntdchr=10
       tdstr(1:ntdchr)='tdnolpyear'
      ELSE IF(Itdtst.eq.4)THEN
       ntdchr=7
       tdstr(1:ntdchr)='td1coef'
      ELSE IF(Itdtst.eq.5)THEN
       ntdchr=11
       tdstr(1:ntdchr)='td1nolpyear'
      ELSE
       IF(Itdtst.eq.6)THEN
        ntdchr=13
        tdstr(1:ntdchr)='tdstock1coef['
       ELSE
        ntdchr=8
        tdstr(1:ntdchr)='tdstock['
       END IF
       ntdchr=ntdchr+1
       CALL itoc(Aicstk,tdstr,ntdchr)
       IF(Lfatal)RETURN
       tdstr(ntdchr:ntdchr)=']'
      END IF
      IF(Aicrgm(1).ne.NOTSET)THEN
       CALL wrtdat(Aicrgm,Sp,datstr,nchdat)
       IF(Lfatal)RETURN
       IF(Tdzero.eq.0)THEN
        tdstr((ntdchr+1):(ntdchr+nchdat+2))='/'//datstr(1:nchdat)//'/'
        ntdchr=ntdchr+nchdat+2
       ELSE IF(Tdzero.eq.1)THEN
        tdstr((ntdchr+1):(ntdchr+nchdat+3))='/'//datstr(1:nchdat)//'//'
        ntdchr=ntdchr+nchdat+3
       ELSE IF(Tdzero.eq.2)THEN
        tdstr((ntdchr+1):(ntdchr+nchdat+4))='//'//datstr(1:nchdat)//'//'
        ntdchr=ntdchr+nchdat+4
       ELSE
        tdstr((ntdchr+1):(ntdchr+nchdat+3))='//'//datstr(1:nchdat)//'/'
        ntdchr=ntdchr+nchdat+3
       END IF
      END IF
c-----------------------------------------------------------------------
      RETURN
      END
      