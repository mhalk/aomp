module gpu_stop_impl
    implicit none
contains
    subroutine stop_with_integer()
        implicit none
        print '(A)', 'Launching kernel'
        !$omp target
            stop 1
        !$omp end target
        print '(A)', 'This should not be printed!'
    end subroutine stop_with_integer
end module gpu_stop_impl

program gpu_stop
    use gpu_stop_impl
    implicit none
    call stop_with_integer()
end program gpu_stop