C     Last change:  BCM  17 Apr 2003   11:09 pm
      SUBROUTINE mstest(Array,Jfda,Jlda,Nyr,Lprt)
      IMPLICIT NONE
c-----------------------------------------------------------------------
C --- AN F TEST FOR MOVING SEASONALITY
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'ssft.cmn'
      INCLUDE 'units.cmn'
      INCLUDE 'hiddn.cmn'
      INCLUDE 'title.cmn'
      INCLUDE 'tests.cmn'
      INCLUDE 'x11opt.cmn'
c-----------------------------------------------------------------------
      DOUBLE PRECISION ONE,ZERO
      PARAMETER(ONE=1D0,ZERO=0D0)
c-----------------------------------------------------------------------
      DOUBLE PRECISION Array,Temp,fvalue,suma1,xbar,colss,colmn,rowss,
     &                 rowmn,fnyr,totss,errss,degfre,fnoyrs,rowssn,
     &                 errssn,c
      CHARACTER bk*2,s1*2,s2*2,fstar*2,xb*50
      INTEGER i,i1,ifmo,j,j1,Jfda,Jlda,k,k1,l,l1,lmo,m,n,n1,ndgfre,
     &        nmin1,noyrs,Nyr,sp1
      LOGICAL Lprt
      DIMENSION Temp(PLEN)
c-----------------------------------------------------------------------
      COMMON /work  / Temp
c-----------------------------------------------------------------------
      DIMENSION Array(Jlda)
c  Bob Fay moved EXTERNAL up
      LOGICAL dpeq
      EXTERNAL dpeq,fvalue
c-----------------------------------------------------------------------
      DATA s1,s2,bk/'* ','**','  '/
c     ------------------------------------------------------------------
      c=ONE
      sp1=0
      IF(Lwdprt)sp1=18
      xb='                                                  '
      ifmo=(Jfda+Nyr-2)/Nyr*Nyr+1
      lmo=Jlda/Nyr*Nyr
      noyrs=(lmo-ifmo)/Nyr+1
      fnoyrs=noyrs
      IF(Muladd.eq.0)THEN
c-----------------------------------------------------------------------
C -- MULTIPLICATIVE MODEL
c-----------------------------------------------------------------------
       c=10000.0D0
       DO j=ifmo,lmo
        Temp(j)=abs(Array(j)-ONE)
       END DO
      ELSE
c-----------------------------------------------------------------------
C --- ADDITIVE MODEL
c-----------------------------------------------------------------------
       DO i=ifmo,lmo
        Temp(i)=abs(Array(i))
       END DO
      END IF
c-----------------------------------------------------------------------
C --- ANALYSIS OF VARIANCE TEST
c-----------------------------------------------------------------------
      suma1=ZERO
      DO k=ifmo,lmo
       suma1=suma1+Temp(k)
      END DO
      fnyr=Nyr
      xbar=suma1/(fnyr*fnoyrs)
      colss=ZERO
      DO l=1,Nyr
       colmn=ZERO
       k1=ifmo+l-1
       DO m=k1,lmo,Nyr
        colmn=colmn+Temp(m)
       END DO
       colmn=colmn/fnoyrs
       colss=colss+(colmn-xbar)*(colmn-xbar)
      END DO
      colss=colss*fnoyrs*c
      rowss=ZERO
      DO n=ifmo,lmo,Nyr
       rowmn=ZERO
       l1=n+Nyr-1
       DO i1=n,l1
        rowmn=rowmn+Temp(i1)
       END DO
       rowmn=rowmn/fnyr
       rowss=rowss+(rowmn-xbar)*(rowmn-xbar)
      END DO
      rowss=rowss*fnyr*c
      totss=ZERO
      DO j1=ifmo,lmo
       totss=totss+(Temp(j1)-xbar)*(Temp(j1)-xbar)
      END DO
      errss=totss*c-colss-rowss
      degfre=fnoyrs-ONE
      rowssn=rowss/degfre
      errssn=errss/((fnyr-ONE)*degfre)
      ndgfre=(Nyr-1)*(noyrs-1)
      IF(dpeq(errssn,ZERO))THEN
       CALL errhdr
       WRITE(Mt1,1001)xb(1:(sp1+12)),xb(1:(sp1+12))
       WRITE(Mt2,1001)' WARNING: ',xb(1:10)
 1001  FORMAT(/,a,'Cannot compute moving F-statistic since residual ',
     &            'mean square',
     &        /,a,'error is equal to zero for this series.')
       RETURN
      END IF
      Fmove=rowssn/errssn
      n1=noyrs-1
      P2=fvalue(Fmove,n1,ndgfre)*100D0
      IF(Issap.eq.2)Ssmf(Icol)=Fmove
c-----------------------------------------------------------------------
      IF(.not.Lprt.or.Lhiddn)RETURN
c-----------------------------------------------------------------------
      IF(P2.le.0.1D0)THEN
       fstar=s2
      ELSE IF(P2.gt.ONE)THEN
       fstar=bk
      ELSE
       fstar=s1
      END IF
      nmin1=noyrs-1
      IF(Lcmpaq)THEN
       WRITE(Mt1,1011)
 1011  FORMAT(/,'  Moving Seasonality Test'/)
       WRITE(Mt1,1021)xb(1:(sp1+21)),xb(1:(sp1+5)),rowss,
     &                nmin1,rowssn,Fmove,fstar,xb(1:(sp1+13)),errss,
     &                ndgfre,errssn
 1021  FORMAT(a,'Sum of squares',2x,'Dgrs.freedom',2x,'Mean square',5x,
     &        'F-value',/,a,'Between Years',
     &        f17.4,i9,f17.6,f12.3,a2,/,a,'Error',f17.4,i9,f17.6,/)
      ELSE
       WRITE(Mt1,1010)xb(1:(sp1+2))
 1010  FORMAT(//,a,'Moving Seasonality Test')
       WRITE(Mt1,1020)xb(1:(sp1+25)),xb(1:(sp1+24)),xb(1:(sp1+3)),rowss,
     &                nmin1,rowssn,Fmove,fstar,xb(1:(sp1+11)),errss,
     &                ndgfre,errssn
 1020  FORMAT(a,'Sum of',5x,'Dgrs.of',9x,'Mean',/,a,'Squares',5x,
     &        'Freedom',8x,'Square',7x,'F-value',/,a,'Between Years',
     &        f17.4,i9,f17.6,f12.3,a2,/,a,'Error',f17.4,i9,f17.6,/)
      END IF
      IF(fstar.ne.bk)THEN
       WRITE(Mt1,1030)xb(1:(sp1+10)),fstar
 1030  FORMAT(a,A2,
     &        'Moving seasonality present at the one percent level.')
       RETURN
      ELSE IF(P2.ge.5D0)THEN
       WRITE(Mt1,1040)xb(1:(sp1+10)),fstar
 1040  FORMAT(a,A2,
     &   'No evidence of moving seasonality at the five percent level.')
       RETURN
      END IF
      WRITE(Mt1,1050)xb(1:(sp1+12))
 1050 FORMAT(a,'Moving seasonality present at the five percent level.')
      RETURN
      END
