C     Last change:  BCM  20 May 1998   11:27 am
**==btrit.f    processed by SPAG 4.03F  at 14:07 on 24 Aug 1994
      SUBROUTINE btrit(Nyearz,Nopt,No2,Iagr,Ext,Eststr,Nstr,Cpobs,
     &                 Lrange,Ssdiff,Lp,Ls)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C  *****  PRINTS OUT SUMMARY BREAKDOWN TABLES FOR TOTAL, MONTHS, YEARS.
C  *****  CALCULATES PERCENTAGE OF MONTHS FLAGGED (FPER), PERCENTAGE OF
C  *****  MONTHS WITH CHANGE IN DIRECTION (SPER), PERCENTAGE OF MONTHS
C  *****  WITH CHANGE OF DIRECTION THAT WERE FLAGGED (SFPER).
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'ssap.cmn'
      INCLUDE 'sspvec.cmn'
c-----------------------------------------------------------------------
      LOGICAL Ls,Lp,Lrange,Ssdiff
      CHARACTER Eststr*(45),Ext*(2),Cpobs*(9),fmt*(35)
      INTEGER Iagr,ij,iy,j,j1,Nstr,next,No2,Nopt,Nyearz,lfmt,nfmt
      DIMENSION Cpobs(20)
c-----------------------------------------------------------------------
      INTEGER nblank
      EXTERNAL nblank
c----------------------------------------------------------------------
      IF(.not.(Lp.or.Ls))RETURN
c----------------------------------------------------------------------
c     Set number of nonblank characters in Ext
c----------------------------------------------------------------------
      next=1
      IF(Iagr.eq.6)next=2
c----------------------------------------------------------------------
c     Print out table header
c----------------------------------------------------------------------
      IF(Lp)THEN
       IF(Ssdiff)THEN
        IF(Lwdprt)THEN
         WRITE(Mt1,1005)Ext,Eststr(1:Nstr),Serno(1:Nser)
 1005    FORMAT(' S  3.',a2,'  Breakdown of Average Maximum Absolute ',
     &          'Differences across spans for ',a,/,10x,'of ',a,'.',/)
        ELSE
         WRITE(Mt1,1006)Ext,Eststr(1:Nstr),Serno(1:Nser)
 1006    FORMAT(' S  3.',a2,'  Breakdown of the Average Maximum ',
     &          'Absolute Differences across spans for ',/,10x,a,' of ',
     &          a,'.',/)
        END IF
       ELSE IF(Lrange)THEN
        IF(Lwdprt)THEN
         WRITE(Mt1,1010)Ext,Eststr(1:Nstr),Eststr(1:Nstr),Serno(1:Nser)
 1010    FORMAT(/,' S  3.',a2,'  Breakdowns of unstable ',a,
     &            ' and Average Maximum Percent Differences',
     &          /,10x,'across spans for ',a,' of ',a,'.',/)
        ELSE
         WRITE(Mt1,1011)Ext,Eststr(1:Nstr),Eststr(1:Nstr),Serno(1:Nser)
 1011    FORMAT(/,' S  3.',a2,'  Breakdowns of unstable ',a,/,10x,
     &            'and Average Maximum Percent Differences across ',
     &            'spans for ',/,10x,a,' of ',a,'.',/)
        END IF
       ELSE
        IF(Lwdprt)THEN
         WRITE(Mt1,1015)Ext,Eststr(1:Nstr),Serno(1:Nser)
 1015    FORMAT(' S  3.',a2,'  Breakdown of Average Maximum Percent ',
     &          'Differences across spans for ',a,/,10x,'of ',a,'.',/)
        ELSE
         WRITE(Mt1,1016)Ext,Eststr(1:Nstr),Serno(1:Nser)
 1016    FORMAT(' S  3.',a2,'  Breakdown of the Average Maximum ',
     &          'Percent Differences across spans for ',/,10x,a,' of ',
     &          a,'.',/)
        END IF
       END IF
       IF(Iagr.eq.6)WRITE(Mt1,1020)
 1020  FORMAT(10x,'Indirect seasonal adjustment',/)
      END IF
c----------------------------------------------------------------------
c     Print out summaries for each period and year
c----------------------------------------------------------------------
      ij=Icyr-Iyr
      IF(Icyr.eq.Iyr)THEN
       IF(No2.eq.3)ij=ij+1
       IF((No2.eq.1.or.No2.eq.2).and.Im.eq.Nsea)ij=ij+1
      END IF
c----------------------------------------------------------------------
      IF(Lp)THEN
       IF(Aobsmx(Nopt).lt.1000D0)THEN
        IF((.not.Ssdiff).and.Lrange)THEN
         fmt='(10X,A9,A3,I3,5X,A8,F5.1,a)'
         nfmt=27
        ELSE
         fmt='(10X,A9,A3,F6.2)'
         nfmt=16
        END IF
       ELSE
        lfmt=idint(dlog10(Aobsmx(Nopt)))+1
        IF((.not.Ssdiff).and.Lrange)THEN
         fmt='(10X,A9,A3,I3,5X,A8,F'
         nfmt=22
         CALL itoc(lfmt+2,fmt,nfmt)
         fmt(nfmt:(nfmt+4))='.1,a)'
         nfmt=nfmt+4
        ELSE
         fmt='(10X,A9,A3,F'
         nfmt=13
         CALL itoc(lfmt+3,fmt,nfmt)
         fmt(nfmt:(nfmt+2))='.2)'
         nfmt=nfmt+2
        END IF
       END IF
       j1=0
       IF(Nsea.eq.4)j1=12
       DO j=1,Nsea
        IF((.not.Ssdiff).and.Lrange)THEN
         WRITE(Mt1,fmt(1:nfmt))Cpobs(j+j1),' : ',SSnobs(j,Nopt),
     &                         '(AMPD = ',Aobs(j,Nopt),')'
        ELSE
         WRITE(Mt1,fmt(1:nfmt))Cpobs(j+j1),' : ',Aobs(j,Nopt)
        END IF
       END DO
       WRITE(Mt1,1040)
 1040  FORMAT(/)
       IF(Ayrmx(Nopt).lt.1000D0)THEN
        IF((.not.Ssdiff).and.Lrange)THEN
         fmt='(10X,I4,5X,A3,I3,5X,A8,F5.1,a)'
         nfmt=30
        ELSE
         fmt='(10X,I4,5X,A3,F6.2)'
         nfmt=19
        END IF
       ELSE
        lfmt=idint(dlog10(Ayrmx(Nopt)))+1
        IF((.not.Ssdiff).and.Lrange)THEN
         fmt='(10X,I4,5X,A3,I3,5X,A8,F'
         nfmt=25
         CALL itoc(lfmt+2,fmt,nfmt)
         fmt(nfmt:(nfmt+4))='.1,a)'
         nfmt=nfmt+4
        ELSE
         fmt='(10X,I4,5X,A3,F'
         nfmt=16
         CALL itoc(lfmt+3,fmt,nfmt)
         fmt(nfmt:(nfmt+2))='.2)'
         nfmt=nfmt+2
        END IF
       END IF
       DO j=ij,Nyearz
        iy=Iyr+j
        IF((.not.Ssdiff).and.Lrange)THEN
         WRITE(Mt1,fmt(1:nfmt))iy,' : ',SSnyr(j,Nopt),'(AMPD = ',
     &                         Ayr(j,Nopt),')'
        ELSE
         WRITE(Mt1,fmt(1:nfmt))iy,' : ',Ayr(j,Nopt)
        END IF
       END DO
c       IF(Lrange)WRITE(Mt1,1060)
c 1060  FORMAT(/,'  AMPD = Average Maximum Percentage Difference')
      END IF
c----------------------------------------------------------------------
c     Save summaries for each period and year
c----------------------------------------------------------------------
      IF(Ls)THEN
       j1=0
       IF(Nsea.eq.4)j1=16
       DO j=1,Nsea
        IF((.not.Ssdiff).and.Lrange)THEN
         WRITE(Nform,1070)Ext(1:next),j,Cpobs(j+j1)(1:3),SSnobs(j,Nopt),
     &                    Aobs(j,Nopt)
        ELSE
         WRITE(Nform,1070)Ext(1:next),j,Cpobs(j+j1)(1:3),0,Aobs(j,Nopt)
        END IF
 1070   FORMAT('s3.',a,'.brk.p',i2.2,': ',A3,1x,I3,2X,E17.10)
       END DO
       DO j=ij,Nyearz
        iy=Iyr+j
        IF((.not.Ssdiff).and.Lrange)THEN
         WRITE(Nform,1080)Ext(1:next),j,iy,SSnyr(j,Nopt),Ayr(j,Nopt)
        ELSE
         WRITE(Nform,1080)Ext(1:next),j,iy,0,Ayr(j,Nopt)
        END IF
 1080   FORMAT('s3.',a,'.brk.y',i2.2,': ',I4,1x,I3,2X,E17.10)
       END DO
      END IF
c----------------------------------------------------------------------
      RETURN
      END
