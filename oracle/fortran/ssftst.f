C     Last change:  BCM  12 Jan 98    8:50 am
**==ssftst.f    processed by SPAG 4.03F  at 12:23 on 21 Jun 1994
      SUBROUTINE ssftst(Ncol,Lprt,Lsav)
      IMPLICIT NONE
c-----------------------------------------------------------------------
      DOUBLE PRECISION NINE,THREE,TWO,SEVEN,ZERO
      PARAMETER (NINE=9D0,THREE=3D0,TWO=2D0,SEVEN=7D0,ZERO=0D0)
c-----------------------------------------------------------------------
      INCLUDE 'srslen.prm'
      INCLUDE 'ssap.prm'
      INCLUDE 'ssft.cmn'
      INCLUDE 'units.cmn'
c-----------------------------------------------------------------------
      CHARACTER qf*(3),span*(6),label*(20),ftfmt*(46)
      INTEGER i,Ncol
      DOUBLE PRECISION ssm7,test1,test2
      LOGICAL Lprt,Lsav
      DIMENSION ssm7(MXCOL),qf(MXCOL),span(MXCOL),label(2)
c-----------------------------------------------------------------------
      DOUBLE PRECISION oldssm7(MXCOL),sstest1(MXCOL),sstest2(MXCOL)
c-----------------------------------------------------------------------
      DATA span/'Span 1','Span 2','Span 3','Span 4'/
      DATA label/'Stable seasonality','Moving seasonality'/
c-----------------------------------------------------------------------
      DO i=1,Ncol
       qf(i)='   '
       test1=ZERO
       test2=ZERO
       IF(Issqf(i).eq.0)qf(i)='yes'
       IF(Issqf(i).eq.1)qf(i)='???'
       IF(Issqf(i).eq.2)qf(i)=' no'
       test1=SEVEN/Ssfts(i)
       test2=(THREE*Ssmf(i))/Ssfts(i)
       oldssm7(i)=sqrt((test1+test2)/2)
       IF(oldssm7(i).gt.THREE)oldssm7(i)=THREE
c-----------------------------------------------------------------------
       sstest1(i)=test1
       sstest2(i)=test2
       IF(test1.gt.NINE)test1=NINE
       IF(test2.gt.NINE)test2=NINE
       ssm7(i)=sqrt((test1+test2)/2)
      END DO
c-----------------------------------------------------------------------
      IF(Lprt)THEN
       WRITE(Mt1,1010)
 1010  FORMAT(//,5x,'Summary of tests for stable and moving ',
     &        'seasonality from table D8 for each span',/)
       ftfmt=' '
       WRITE(ftfmt,1020)Ncol
 1020  FORMAT('(28x,',i1,'(7x,a6))')
       WRITE(Mt1,ftfmt)(span(i),i=1,Ncol)
c-----------------------------------------------------------------------
       ftfmt=' '
       WRITE(ftfmt,1030)Ncol
 1030  FORMAT('(5x,a20,6x,',i1,'(2x,f8.2,3x))')
       WRITE(Mt1,ftfmt)label(1),(Ssfts(i),i=1,Ncol)
       WRITE(Mt1,1040)
 1040  FORMAT(' ')
       WRITE(Mt1,ftfmt)label(2),(Ssmf(i),i=1,Ncol)
       WRITE(Mt1,1040)
c-----------------------------------------------------------------------
       ftfmt=' '
       WRITE(ftfmt,1050)Ncol
 1050  FORMAT('(5x,a,24x,',i1,'(2x,f8.2,3x))')
       WRITE(Mt1,ftfmt)'m7',(ssm7(i),i=1,Ncol)
       WRITE(Mt1,1040)
c-----------------------------------------------------------------------
       ftfmt=' '
       WRITE(ftfmt,1060)Ncol
 1060  FORMAT('(5x,a,',i1,'(8x,a3,2x))')
       WRITE(Mt1,ftfmt)'Identifiable seasonality?',(qf(i),i=1,Ncol)
       WRITE(Mt1,1040)
c-----------------------------------------------------------------------
       WRITE(Mt1,1070)
 1070  FORMAT(/,10x,'yes = Identifiable seasonality probably present',/,
     &        10x,'??? = Identifiable seasonality probably not present',
     &        /,10x,' no = Identifiable seasonality not present',//)
      END IF
c-----------------------------------------------------------------------
      IF(Lsav)THEN
       ftfmt=' '
       WRITE(ftfmt,1080)Ncol
 1080  FORMAT('(a,',i1,'(3x,f8.2))')
       WRITE(Nform,ftfmt)'ssfstab:',(Ssfts(i),i=1,Ncol)
       WRITE(Nform,ftfmt)'ssfmov:',(Ssmf(i),i=1,Ncol)
*       WRITE(Nform,ftfmt)'sstest1:',(sstest1(i),i=1,Ncol)
*       WRITE(Nform,ftfmt)'sstest2:',(sstest2(i),i=1,Ncol)
       WRITE(Nform,ftfmt)'ssm7:',(ssm7(i),i=1,Ncol)
*       WRITE(Nform,ftfmt)'oldssm7:',(oldssm7(i),i=1,Ncol)
       ftfmt=' '
       WRITE(ftfmt,1090)Ncol
 1090  FORMAT('(a,',i1,'(8x,a3))')
       WRITE(Nform,ftfmt)'ssident:',(qf(i),i=1,Ncol)
      END IF
c-----------------------------------------------------------------------
      RETURN
      END
