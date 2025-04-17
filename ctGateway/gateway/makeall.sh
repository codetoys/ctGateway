

#################################################################################

#如果出错则退出，参数1为退出码
function OnErrorExit
{
	if [[ $? -ne 0 ]]
	then
		echo 出错，错误码 $1
		exit $1
	fi
}

#################################################################################

pwd

mkdir bin

#定义目标目录列表
MAKE_DIR_LIST="gwBuiltIn gwprotocol gwmain"

#引入函数文件
. ../third/ctfc/platform/_make_dir_list.sh
OnErrorExit $?

#编译目标列表
make_dir_list
OnErrorExit $?

echo ${MAKE_DIR_LIST}
echo 检查结果。。。。。。
date
ls -l ../lib
ls -l bin/

file bin/*
