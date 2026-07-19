C     ref_mltpos.f -- reference output for mltpos (needs srslen/model.prm + copy).
C       gfortran -O2 -Ioracle/fortran tools/ref_mltpos.f \
C         oracle/fortran/mltpos.f oracle/fortran/copy.f -o ref_mltpos && ./ref_mltpos
      PROGRAM ref_mltpos
      IMPLICIT NONE
      DOUBLE PRECISION arimap(25),ca(5),cb(5)
      INTEGER arimal(25),opr(0:9),i

C     Case A: single AR op lag1 phi=0.5, nelta=3, neltc=5, c=[1,2,3,0,0]
      opr(0)=1
      opr(1)=2
      arimal(1)=1
      arimap(1)=0.5D0
      ca(1)=1.0D0
      ca(2)=2.0D0
      ca(3)=3.0D0
      ca(4)=0.0D0
      ca(5)=0.0D0
      CALL mltpos(3,arimap,arimal,opr,1,1,5,ca)
      DO i=1,5
       WRITE(*,'(A,I1,A,ES24.16)') 'mltA',i,' ',ca(i)
      END DO

C     Case B: two AR ops (phi=0.5 then phi=0.3), exercises secpas=true pass
      opr(0)=1
      opr(1)=2
      opr(2)=3
      arimal(1)=1
      arimap(1)=0.5D0
      arimal(2)=1
      arimap(2)=0.3D0
      cb(1)=1.0D0
      cb(2)=2.0D0
      cb(3)=3.0D0
      cb(4)=0.0D0
      cb(5)=0.0D0
      CALL mltpos(3,arimap,arimal,opr,1,2,5,cb)
      DO i=1,5
       WRITE(*,'(A,I1,A,ES24.16)') 'mltB',i,' ',cb(i)
      END DO
      END
