*libgcc:
%{mthreads:-lmingwthrd} -lmingw32      %{static|static-libgcc:-lgcc -lgcc_eh}  %{!static:    %{!static-libgcc:      %{!shared:        %{!shared-libgcc:-lgcc -lgcc_eh}        %{shared-libgcc:-lgcc_s -lgcc}       }      %{shared:-lgcc_s -lgcc}     }   }     -lmingwex -lmsvcr100

