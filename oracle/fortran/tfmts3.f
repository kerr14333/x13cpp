C     Last change:  BCM  22 Dec 97    4:32 pm
      SUBROUTINE tfmts3(Outdec,Muladd,Tblwid,Lwidpr,Ifmt3)
      IMPLICIT NONE
c-----------------------------------------------------------------------
c     Generate format for summary statistics at the end of the printout
c-----------------------------------------------------------------------
      INCLUDE 'error.cmn'
c-----------------------------------------------------------------------
      CHARACTER Ifmt3*132,wid*2,wid2*2
      LOGICAL Lwidpr
      INTEGER Outdec,fac,Muladd,ipos,ipos2,Tblwid,wid3
c-----------------------------------------------------------------------
      fac=Outdec
      IF(Muladd.eq.0.or.Muladd.eq.2.and.fac.eq.0)fac=2
      ipos=1
      ipos2=1
      CALL itoc(Tblwid+2,wid,ipos)
      IF(.not.Lfatal)CALL itoc(Tblwid+4,wid2,ipos2)
      IF(Lfatal)RETURN
      ipos=ipos-1
      ipos2=ipos2-1
      IF(Lwidpr)THEN
       wid3=Tblwid+40
       WRITE(Ifmt3,1010)wid2(1:ipos2),fac,wid(1:ipos),fac,wid(1:ipos),
     &                  fac,wid3,wid(1:ipos),fac,wid(1:ipos),fac
 1010  FORMAT('(/,15x,''Table Total- '',f',a,'.',i1,',8x,''Mean- '',f',
     &        a,'.',i1,',8x,''Std. Deviation- '',f',a,'.',i1,',/,',i2,
     &      'x,''Min - '',f',a,'.',i1,',18x,''Max - '',f',a,'.',i1,')')
      ELSE
       wid3=Tblwid+22
       WRITE(Ifmt3,1020)wid2(1:ipos2),fac,wid(1:ipos),fac,wid(1:ipos),
     &                  fac,wid3,wid(1:ipos),fac,wid(1:ipos),fac
 1020  FORMAT('(/, 2x,''Table Total- '',f',a,'.',i1,',3x,''Mean- '',f',
     &        a,'.',i1,',3x,''Std. Dev.- '',f',a,'.',i1,',/,',i2,
     &      'x,''Min - '',f',a,'.',i1,',8x,''Max - '',f',a,'.',i1,')')
      END IF
c-----------------------------------------------------------------------
      RETURN
      END
