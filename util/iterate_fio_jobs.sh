#!/usr/bin/env bash

cleanup() {
    # Attempt to cleanup a leftover FIO results file
    # at the end of this process.
    local rc=$?
    if [ -f ${fio_single_job_results_path} ]; then
        rm ${fio_single_job_results_path}
        echo "Cleaned up FIO log file."
    fi
    exit ${rc}
}
trap cleanup EXIT INT TERM

quick_copy_results="IO Mode,RW Operation,Block Size,Bandwidth (bytes/s),Bandwidth (GiB/s),Avg Latency (ns),User CPU%,Sys CPU%,Total CPU%\n"
pidstat_copy_results=""

fio_single_job_results_path="/tmp/fio_job_results.json"
fio_runtime=30
fio_ramptime=5
fio_filesize="1GiB"
#fio_data_directory="/mnt/ais/ext4/fio_jobs"  # NVMe drive locally on Rowlet
fio_data_directory="/mnt/ais/pancham_ais_ext4/fio_data"  # NVMeoF drive on Pancham
fio_ioengines=("libcufile" "librocfile")
fio_gpu_io_methods=("posix" "_gpu_io")
fio_rw_modes=("read" "write")
block_sizes=("4KiB" "16KiB" "64KiB" "256KiB" "1MiB" "4MiB" "16MiB" "64MiB" "256MiB" "1GiB")

for ioengine in "${fio_ioengines[@]}"; do
    for gpu_io_method in "${fio_gpu_io_methods[@]}"; do
        for rw_mode in "${fio_rw_modes[@]}"; do
            for size in "${block_sizes[@]}"; do
                # Ideally any ioengine specific args somehow get processed separately.
                # But for purposes of this helper script where our benchmarks rely on
                # these specific ioengines, we probably are okay.
                _gpu_io_param=""
                _gpu_io_value=""
                _results_gpu_io_mode=""
                if [ "${ioengine}" == "libcufile" ]; then
                    _gpu_io_param="cuda_io"
                    _results_gpu_io_mode="cuFile"
                elif [ "${ioengine}" == "libhipfile" ]; then
                    _gpu_io_param="rocm_io"
                    _results_gpu_io_mode="hipFile"
                else
                    echo "Unknown IOEngine for GPU IO."
                    exit 1
                fi
                if [ "${gpu_io_method}" == "_gpu_io" ] && [ "${ioengine}" == "libcufile" ]; then
                    _gpu_io_value="cufile"
                    _results_gpu_io_mode+="_GDS"
                elif [ "${gpu_io_method}" == "_gpu_io" ] && [ "${ioengine}" == "libhipfile" ]; then
                    _gpu_io_value="hipFile"
                    _results_gpu_io_mode+="_AIS"
                elif [ "${gpu_io_method}" == "posix" ]; then
                    _gpu_io_value="posix"
                    _results_gpu_io_mode+="_POSIX"
                else
                    echo "Unknown GPU IO Method."
                    exit 1
                fi

                echo "FIO Job Config: ${_results_gpu_io_mode} ${rw_mode} ${size}"
                #fio_job_name="${ioengine}_${gpu_io_method}_${rw_mode}_${size}"
                # Keeping job name constant in order to minimize the number of data files
                # FIO generates/writes to. Not as big of an issue if FIO cleaned these up
                # at the end of each job. Do we need to worry about drive cache?
                fio_job_name="NVMeoF_Benchmark"
                fio_cmd="/home/rildixon/rocfile/fio/fio --warnings-fatal --output-format=json "
                fio_cmd+="--name=${fio_job_name} --kb_base=1000 --directory=${fio_data_directory} "
                fio_cmd+="--numjobs=1 --time_based --runtime=${fio_runtime} --ramp_time=${fio_ramptime} --thread "
                fio_cmd+="--cpus_allowed=15 --direct=1 --rw=${rw_mode} --size=${fio_filesize} "
                fio_cmd+="--ioengine=${ioengine} --${_gpu_io_param}=${_gpu_io_value} --gpu_dev_ids=0 "
                fio_cmd+="--bs=${size} > ${fio_single_job_results_path} &"
                #echo "fio invocation: ${fio_cmd}"
                eval $fio_cmd
                FIO_PID=$!
                echo "FIO_PID: ${FIO_PID}"
                pidstat_output=$(pidstat -u -p ${FIO_PID} 1 $((${fio_runtime}+${fio_ramptime})))
                pidstat_output=$(echo "${pidstat_output}" | awk 'NR > 3 && !/Average:/ {print}')
                pidstat_copy_results+="${pidstat_output}\n\n"
                wait ${FIO_PID}
                fio_exit_code=$?
                if [ ${fio_exit_code} -ne 0 ]; then
                    echo "FIO Job Failed. Logging failure and continuing."
                    quick_copy_results+="${_results_gpu_io_mode},${rw_mode},${size},0,0,0,0,0,0\n"
                    continue
                fi
                fio_output=$(<${fio_single_job_results_path})
                read_bw=$(echo ${fio_output} | jq '.jobs[0].read.bw_bytes')
                read_lat=$(echo ${fio_output} | jq '.jobs[0].read.lat_ns.mean')
                write_bw=$(echo ${fio_output} | jq '.jobs[0].write.bw_bytes')
                write_lat=$(echo ${fio_output} | jq '.jobs[0].write.lat_ns.mean')
                usr_cpu=$(echo ${fio_output} | jq '.jobs[0].usr_cpu')
                sys_cpu=$(echo ${fio_output} | jq '.jobs[0].sys_cpu')
                total_cpu=$(echo "scale=2; ${usr_cpu} + ${sys_cpu}" | bc) # Add two FP numbers together, precision of 2.
                if [ "${read_bw}" -eq 0 ]; then
                    write_bw_gib=$(echo "scale=8; ${write_bw} / 2^30" | bc)
                    quick_copy_results+="${_results_gpu_io_mode},${rw_mode},${size},${write_bw},${write_bw_gib},"
                    quick_copy_results+="${write_lat},${usr_cpu},${sys_cpu},${total_cpu}\n"
                else
                    read_bw_gib=$(echo "scale=8; ${read_bw} / 2^30" | bc)
                    quick_copy_results+="${_results_gpu_io_mode},${rw_mode},${size},${read_bw},${read_bw_gib},"
                    quick_copy_results+="${read_lat},${usr_cpu},${sys_cpu},${total_cpu}\n"
                fi
            done
        done
    done
done

echo -e "Quick results:\n${quick_copy_results}"
echo -e "Pidstat Data:\n${pidstat_copy_results}"

