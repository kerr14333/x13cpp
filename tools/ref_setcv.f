      PROGRAM refsetcv
      IMPLICIT NONE
      DOUBLE PRECISION setcv,setcvl
      WRITE(*,'(A,1PE24.16)') 'setcv132= ',setcv(132,0.5D0)
      WRITE(*,'(A,1PE24.16)') 'setcv60= ',setcv(60,0.5D0)
      WRITE(*,'(A,1PE24.16)') 'setcvl100= ',setcvl(100,0.5D0)
      END
      SUBROUTINE writln(a,b,c,d)
      CHARACTER a*(*)
      INTEGER b,c
      LOGICAL d
      RETURN
      END
