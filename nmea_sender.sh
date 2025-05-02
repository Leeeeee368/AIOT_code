#!/bin/bash

# 定义串口设备
SERIAL_PORT="/dev/pts/9"

# 更可靠的校验和计算函数
calculate_checksum() {
    local nmea_string=$1
    local checksum=0

    # 计算从$后第一个字符到*前的所有字符的异或值
    for (( i=1; i<${#nmea_string}; i++ )); do
        char="${nmea_string:$i:1}"
        if [[ "$char" == "*" ]]; then
            break
        fi
        checksum=$((checksum ^ $(printf '%d' "'$char")))
    done

    printf "%02X" $checksum
}

# 生成随机GGA语句（带校验和验证）
generate_gga() {
    local time=$(date -u +%H%M%S.%3N | cut -b1-10)
    local lat_deg=$((38 + RANDOM % 2))
    local lat_min=$((52 + RANDOM % 8)).$(shuf -i 100000-999999 -n 1)
    local lon_deg=$((117 + RANDOM % 2))
    local lon_min=$((8 + RANDOM % 50)).$(shuf -i 100000-999999 -n 1)
    local fix=$((1 + RANDOM % 3))
    local satellites=$((4 + RANDOM % 8))
    local hdop=$(awk -v min=1.0 -v max=5.0 'BEGIN{srand(); printf "%.2f\n", min+rand()*(max-min)}')
    local altitude=$(awk -v min=10.0 -v max=50.0 'BEGIN{srand(); printf "%.3f\n", min+rand()*(max-min)}')
    local geoid_sep=$(awk -v min=-10.0 -v max=10.0 'BEGIN{srand(); printf "%.3f\n", min+rand()*(max-min)}')

    # 构建GGA语句（不含$和*）
    local gga_body="GNGGA,${time},${lat_deg}${lat_min},N,${lon_deg}${lon_min},E,${fix},${satellites},${hdop},${altitude},M,${geoid_sep},M,,"

    # 计算校验和
    local checksum=$(calculate_checksum "\$${gga_body}*")

    # 返回完整语句
    echo "\$${gga_body}*${checksum}"
}

# 生成随机RMC语句（带校验和验证）
generate_rmc() {
    local time=$(date -u +%H%M%S.%3N | cut -b1-10)
    local date=$(date -u +%d%m%y)
    local lat_deg=$((38 + RANDOM % 2))
    local lat_min=$((52 + RANDOM % 8)).$(shuf -i 100000-999999 -n 1)
    local lon_deg=$((117 + RANDOM % 2))
    local lon_min=$((8 + RANDOM % 50)).$(shuf -i 100000-999999 -n 1)
    local speed=$(awk -v min=0.0 -v max=1.0 'BEGIN{srand(); printf "%.3f\n", min+rand()*(max-min)}')
    local course=$(awk -v min=0.0 -v max=359.9 'BEGIN{srand(); printf "%.2f\n", min+rand()*(max-min)}')

    # 构建RMC语句（不含$和*）
    local rmc_body="GNRMC,${time},A,${lat_deg}${lat_min},N,${lon_deg}${lon_min},E,${speed},${course},${date},,,A,V"

    # 计算校验和
    local checksum=$(calculate_checksum "\$${rmc_body}*")

    # 返回完整语句
    echo "\$${rmc_body}*${checksum}"
}

# 验证NMEA语句的函数
validate_nmea() {
    local nmea=$1
    local calculated_cs=$(calculate_checksum "$nmea")
    local provided_cs=$(echo "$nmea" | grep -o '\*[0-9A-Fa-f][0-9A-Fa-f]$' | tr -d '*')

    if [[ "${calculated_cs}" != "${provided_cs}" ]]; then
        echo "ERROR: Checksum mismatch for $nmea" >&2
        echo "Calculated: $calculated_cs, Provided: $provided_cs" >&2
        return 1
    fi
    return 0
}

# 主循环
while true; do
    # 生成语句
    gga=$(generate_gga)
    rmc=$(generate_rmc)

    # 验证校验和
    if ! validate_nmea "$gga" || ! validate_nmea "$rmc"; then
        echo "生成语句校验和错误，跳过本次发送" >&2
        sleep 1
        continue
    fi

    # 发送到串口
    echo -e "${gga}\n${rmc}" > "$SERIAL_PORT"
    echo "Sent:"
    echo -e "${gga}\n${rmc}\n"

    sleep 1
done
