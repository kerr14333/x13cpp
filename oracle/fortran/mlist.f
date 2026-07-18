C     Last change:  BCM  26 Feb 1999    3:40 pm
**==mlist.f    processed by SPAG 4.03F  at 12:23 on 21 Jun 1994
      SUBROUTINE mlist(X,Nopt,Nop2,Dmax,N48,Iagr,Ext,Eststr,Nstr,Ncol,Y,
     &                 Period,Ssdiff)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C  *****  PRINTS OUT EACH OBSERVATION IN SLIDING SPANS ANALYSIS, WITH
C  *****  DATE, ESTIMATES (EXAMPLE, SEASONAL FACTORS) FOR EACH SPAN,
C  *****  MAXIMUM PERCENTAGE DIFFERENCE (DMAX), AND AN INDICATION OF
C  *****  WHETHER THE OBSERVATION WAS FLAGGED AS AN EXTREME (PER)
C  *****  OR CHANGED DIRECTION (CSIGN).
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'notset.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'ssap.cmn'
      INCLUDE 'sspvec.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'title.cmn'
c-----------------------------------------------------------------------
      DOUBLE PRECISION Dmax,X
      LOGICAL Ssdiff,l2Big
      CHARACTER cagr*(31),dash*(1),Eststr*(45),Ext*(2),f*(7),blank8*(8),
     &          cfirst*(11),fnotvc*(10)
      INTEGER i,Iagr,iy,l,l0,l1,l2,Nstr,m,N48,Nop2,Nopt,Y,Period,nagr,
     &        Ncol,nfirst,nc,nt,fnotky,nssky
      DIMENSION dash(3),X(MXLEN,MXCOL),Dmax(MXLEN,NEST),f(3),Y(2*MXCOL),
     &          Period(2*MXCOL),nfirst(2),cfirst(2),fnotvc(MXLEN),
     &          fnotky(7)
c-----------------------------------------------------------------------
      LOGICAL dpeq
      EXTERNAL dpeq
c-----------------------------------------------------------------------
      DATA dash/'-','/',' '/
      DATA f/'MAXIMUM',' % DIFF','   DIFF'/
      DATA cfirst/'seasonal','trading day'/
      DATA nfirst/8,11/
c-----------------------------------------------------------------------
      iy=Iyr
      m=Im-1
      IF(Iagr.eq.5)THEN
       cagr=': Direct seasonal adjustment.'
       nagr=29
      ELSE IF(Iagr.eq.6)THEN
       cagr=': Indirect seasonal adjustment.'
       nagr=31
      ELSE
       cagr='.'
       nagr=1
      END IF
      blank8='        '
c-----------------------------------------------------------------------
c  Check to see if series is too large to be printed - if so,
c  switch to scientific format.
c  added by BCM Dec 2005
c-----------------------------------------------------------------------
      l2Big=.false.
      IF(Nopt.ge.3.or.Ssdiff)THEN
       DO l1=1,Ncol
        DO l2=Im,Sslen+Im-1
         IF(.not.dpeq(X(l2,l1),DNOTST))THEN
          IF(X(l2,l1).gt.999999.99 .or. X(l2,l1).lt.-99999.99)
     &       l2Big=.true.
          IF(l2Big)GO TO 1000
         END IF
        END DO
       END DO
      END IF
 1000 CONTINUE
c-----------------------------------------------------------------------
c     Generate footnotes for table (BCM, December 2006)
c-----------------------------------------------------------------------
      CALL ssfnot(Nopt,Nop2,fnotvc,fnotky,nssky)
c-----------------------------------------------------------------------
c     Print out complete sliding spans information, with up to 48
c     observations on a page.
c-----------------------------------------------------------------------
      DO l0=1,N48
       l1=(l0-1)*48+Im
       l2=l0*48+Im-1
       IF(l0.eq.N48)l2=Sslen+Im-1
       IF(Lpage)THEN
        WRITE(Mt1,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Serno(1:Nser)
        Kpage=Kpage+1
       END IF
       WRITE(Mt1,1020)Ext,Eststr(1:Nstr),Serno(1:Nser),cagr(1:nagr)
 1020  FORMAT(' S  7.',a2,'  Sliding spans analysis of ',a,' for ',a,a)
       WRITE(Mt1,1030)
       WRITE(Mt1,F2)(Period(i),dash(2),Y(i),dash(1),i=1,Ncol),f(1),
     &               blank8
       IF(Nop2.eq.0.AND.(.not.Ssdiff))THEN
        WRITE(Mt1,F2)(Period(Ncol+i),dash(2),Y(Ncol+i),dash(3),
     &                i=1,Ncol),f(2),'Footnote'
       ELSE
        WRITE(Mt1,F2)(Period(Ncol+i),dash(2),Y(Ncol+i),dash(3),
     &                i=1,Ncol),f(3),'Footnote'
       END IF
       WRITE(Mt1,1030)
 1030  FORMAT(' ')
       nc=0
       nt=0
       DO l=l1,l2
        m=m+1
        IF(m.gt.Nsea)THEN
         m=1
         iy=iy+1
        END IF
        CALL wrtmss(m,iy,X,Dmax,Ncol,Nopt,l,fnotvc(l),l2big)
       END DO
      END DO
c-----------------------------------------------------------------------
      IF(nssky.gt.0)THEN
c-----------------------------------------------------------------------
c  Print header for footnotes on separate page
c-----------------------------------------------------------------------
       IF(Lpage)THEN
        WRITE(Mt1,Ttlfmt)Newpg,Title(1:Ntitle),Kpage,Serno(1:Nser)
        Kpage=Kpage+1
       END IF
       WRITE(Mt1,1040)Ext,Eststr(1:Nstr),Serno(1:Nser),cagr(1:nagr)
 1040  FORMAT('  Footnotes for Table S7.',a2,':',/,
     &        '  Sliding spans analysis of ',a,' for ',a,a,/)
       CALL mkssky(fnotky,nssky,Nopt,Nop2)
       WRITE(Mt1,1030)
      END IF
c-----------------------------------------------------------------------
      RETURN
      END

