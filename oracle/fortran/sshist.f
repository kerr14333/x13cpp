C     Last change:  BCM   5 Jan 1999   11:51 am
      SUBROUTINE sshist(Dmax,Nopt,Iagr,Ext,Iext,Eststr,Nstr,Lrange,Lp,
     &                  Ls,Lpspan,Lwdprt,Ssdiff)
      IMPLICIT NONE
c-----------------------------------------------------------------------
      LOGICAL F,T
      PARAMETER(F=.false.,T=.true.)
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'error.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'ssap.cmn'
      INCLUDE 'x11opt.cmn'
      INCLUDE 'mq3.cmn'
c-----------------------------------------------------------------------
      LOGICAL Lp,Ls,Lwdprt,Lrange,Lpspan,first,Ssdiff
      CHARACTER c*(7),Ext*(2),Eststr*(45)
      DOUBLE PRECISION Dmax,xo
      INTEGER i,it,l2,n,n1,n2,Nopt,Iagr,Nstr,Iext,nmq
      DIMENSION Dmax(MXLEN,NEST),xo(MXLEN)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      INTEGER nblank
      EXTERNAL dpeq,nblank
c-----------------------------------------------------------------------
      l2=Sslen2-1
      it=0
      n2=Ic
      first=T
      DO i=Ic,l2
       IF(dpeq(Dmax(i,Nopt),DNOTST))THEN
        it=it+1
        IF(first)n2=n2+1
       ELSE IF(mod(i,Nsea).eq.2.and.Nopt.eq.2.and.
     &         dpeq(Dmax(i,Nopt),0D0))THEN
        it=it+1
        IF(first)n2=n2+1
       ELSE
        n=i-it-Ic+1
        xo(n)=Dmax(i,Nopt)
        IF(first)first=F
       END IF
      END DO
c-----------------------------------------------------------------------
      n1=1
      IF(Iagr.eq.6)n1=2
c-----------------------------------------------------------------------
      IF(Muladd.eq.1.and.Ssdiff)THEN
       CALL histx(xo,n,Muladd,Nsea,Iyr,n2,Iext,Ssdiff,Lp,Ls,
     &            'Maximum Absolute Differences across spans')
      ELSE
       CALL histx(xo,n,Muladd,Nsea,Iyr,n2,Iext,F,Lp,Ls,
     &            'Maximum Percent Differences across spans')
      END IF
      IF(Lfatal)RETURN
c-----------------------------------------------------------------------
c     Compute and print out "tail" histogram.
c-----------------------------------------------------------------------
      IF((.not.Ssdiff).and.Lp.and.Lrange)THEN
       nmq=nblank(Moqu)
       IF(Lwdprt)THEN
        WRITE(Mt1,1010)Eststr(1:Nstr),Moqu(1:nmq)
       ELSE
        WRITE(Mt1,1020)Eststr(1:Nstr),Moqu(1:nmq)
       END IF
       c=' '
       IF(Lpspan)c(4:6)=Ch(Nopt)//' :'
       DO i=1,3
        IF(Lpspan)WRITE(c(3:3),1030)i
        WRITE(Mt1,1040)c,Cut(Nopt,i),Cut(Nopt,i+1),Kount(Nopt,i)
       END DO
       IF(Lpspan)c(3:3)='4'
       WRITE(Mt1,1050)c,Cut(Nopt,4),Kount(Nopt,4)
      END IF
c-----------------------------------------------------------------------
      IF((.not.Ssdiff).and.Ls)THEN
       DO i=1,3
        WRITE(Nform,1060)Ext(1:n1),i,Cut(Nopt,i),Cut(Nopt,i+1),
     &                   Kount(Nopt,i)
       END DO
       WRITE(Nform,1070)Ext(1:n1),Cut(Nopt,4),Kount(Nopt,4)
      END IF
c-----------------------------------------------------------------------
 1010 FORMAT(/,'  Breakdown of the maximum percentage differences ',
     &         'of the ',a,/,'  for flagged ',a,'s.',/)
 1020 FORMAT(/,'  Breakdown of the maximum percentage differences ',
     &         'of the',/,2x,a,' for flagged ',a,'s.',/)
 1030 FORMAT(i1)
 1040 FORMAT('  ',a,' Greater than or equal to ',f4.1,'% but less ',
     &       'than ',f4.1,'% :',1x,i3)
 1050 FORMAT('  ',a,' Greater than or equal to ',f4.1,'%',t62,':',1x,
     &       i3,/)
 1060 FORMAT('s3.',a,'.thist',i1,':',2x,f4.1,2x,f4.1,2x,i3)
 1070 FORMAT('s3.',a,'.thist4:',2x,f4.1,8x,i3)
c-----------------------------------------------------------------------
      RETURN
      END
