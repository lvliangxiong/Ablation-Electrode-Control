// 用于是否开启调试
#define ELECTRODE_DEBUG 1;

#define SERVO_ID1 1
#define SERVO_ID2 2
#define SERVO_ID3 3
#define SERVO_ID4 4
#define SERVO_ID5 5
#define SERVO_ID6 6

// Serial 1 的 RX 的 PIN 为 D19
// Serial 1 的 TX 的 PIN 为 D18
#define CONTROL_SERIAL Serial1

#define MSG_LEN 80

// 存放 servo 的信息
struct servo
{
    // 舵机 id
    uint8_t id;
    // 装配时，舵机的位置
    int16_t assemblyPosition;
    // 电极完全收回时，舵机的位置
    int16_t withdrawPosition;
};

// 存放 一组子电极 的信息，包括外套管和导丝
struct sub_electrode
{
    // 子电极的编号 id
    uint8_t id;
    // 子电极的外套管对应的舵机的 id
    uint8_t cannulaServoID;
    // 子电极的导丝对应的舵机的 id
    uint8_t styletServoID;
};

// 定义舵机和电极的一些运动常数
// 外套管的行程，单位 mm
const double CANNULA_STROKE = 20;
// 导丝的行程，单位 mm
const double STYLET_STROKE = 52;
// 外套管的运动行程，也就是 20mm 对应的舵机的步数
const uint16_t CANNULA_STROKE_SERVO_STEPS = 382;
// 导丝的运动行程，也就是 52mm 对应的舵机的步数
const uint16_t STYLET_STROKE_SERVO_STEPS = 993;
// 舵机可用的最大步数
const uint16_t SERVO_POSITION_RANGE_MAX = 1000;
// 舵机可用的最小步数
const uint16_t SERVO_POSITION_RANGE_MIN = 0;

servo servos[6];
sub_electrode sub_electrodes[3];

/*
初始化舵机

舵机编号  装配位置  完全收回位置     |    舵机编号  装配位置   完全收回位置
  1        0        382                3       1000    1000-382=618
  2        0        993                4       1000    1000-993=7
  5        0        382                6       1000    1000-993=7
*/
void ServoDefinition()
{
    // 第一组电极
    servos[0].id = 1;
    servos[0].assemblyPosition = SERVO_POSITION_RANGE_MIN;
    servos[0].withdrawPosition = servos[0].assemblyPosition + CANNULA_STROKE_SERVO_STEPS;

    servos[1].id = 2;
    servos[1].assemblyPosition = SERVO_POSITION_RANGE_MIN;
    servos[1].withdrawPosition = servos[1].assemblyPosition + STYLET_STROKE_SERVO_STEPS;

    // 第二组电极
    servos[2].id = 3;
    servos[2].assemblyPosition = SERVO_POSITION_RANGE_MAX;
    servos[2].withdrawPosition = servos[2].assemblyPosition - CANNULA_STROKE_SERVO_STEPS;

    servos[3].id = 4;
    servos[3].assemblyPosition = SERVO_POSITION_RANGE_MAX;
    servos[3].withdrawPosition = servos[3].assemblyPosition - STYLET_STROKE_SERVO_STEPS;

    // 第三组电极
    servos[4].id = 5;
    servos[4].assemblyPosition = SERVO_POSITION_RANGE_MIN;
    servos[4].withdrawPosition = servos[4].assemblyPosition + CANNULA_STROKE_SERVO_STEPS;

    servos[5].id = 6;
    servos[5].assemblyPosition = SERVO_POSITION_RANGE_MAX;
    servos[5].withdrawPosition = servos[5].assemblyPosition - STYLET_STROKE_SERVO_STEPS;
}

/* 初始化电极和舵机的对应关系 */
void ElectrodeDefinition()
{
    // 左电极的定义
    sub_electrodes[0].id = 1;
    sub_electrodes[0].cannulaServoID = 1;
    sub_electrodes[0].styletServoID = 2;

    // 右电极的定义
    sub_electrodes[1].id = 2;
    sub_electrodes[1].cannulaServoID = 3;
    sub_electrodes[1].styletServoID = 4;

    // 中间电极的定义
    sub_electrodes[2].id = 3;
    sub_electrodes[2].cannulaServoID = 5;
    sub_electrodes[2].styletServoID = 6;
}

void PrintNChar(char ch, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        Serial.print(ch);
    }
}

void PrintSeperatingLine(char ch)
{
    PrintNChar(ch, MSG_LEN);
    Serial.println();
}

void PrintInfo(String str, char padding)
{
    size_t len = str.length();
    size_t left = (MSG_LEN - len) >> 1;
    size_t right = MSG_LEN - left - len;
    PrintNChar(padding, left);
    Serial.print(str);
    PrintNChar(padding, right);
    Serial.println();
}

/*
检查目标位置数组的合理性

输入参数 position[6] 中：
position[0]，position[2]，position[4] 是外套管的目标位置
position[1]，position[3]，position[5] 是导丝的目标位置
 */
bool CheckDestinationPosition(double position[6])
{
    for (size_t i = 0; i < 3; i++)
    {
        size_t cannula = i << 1;
        size_t stylet = cannula + 1;
        // 外套管的位移需要在 [0, 20] mm 之间
        if (position[cannula] < 0.0 || position[cannula] > CANNULA_STROKE)
        {
            PrintInfo("Cannula out of the range, please check!", '*');
            return false;
        }
        // 导丝的位移需要在 [0, 52] mm 之间
        if (position[stylet] < 0.0 || position[stylet] > STYLET_STROKE)
        {
            PrintInfo("Stylet out of the range, please check!", '*');
            return false;
        }
        // 限制stylet的位置，最多只能刚好完全收回,当然也不能超过cannula的限位
        if (position[stylet] < position[cannula] ||
            position[stylet] > position[cannula] + (STYLET_STROKE - CANNULA_STROKE))
        {
            PrintInfo("Stylet out of the range, please check!", '*');
            return false;
        }
    }
    return true;
}

/*
外套管移动函数
输入：
    id            外套管对应的子电极 id
    position      外套管绝对位置(range: [0, 20], unit: mm)
    time          运动时间
*/
void CannulaMove(uint8_t id, double position, uint16_t time)
{
    // 将目标位置限制在 [0, 20] mm 之间
    position = constrain(position, 0.0, 20.0);

    // 获得该外套管对应的舵机的 id
    uint8_t servoID = sub_electrodes[id - 1].cannulaServoID;
    int step;

    // 根据舵机的初始装配位置、完全收回位置和外套管的真实位置进行映射，找到指定外套管位置对应的舵机步数
    if (id == 1 || id == 2 || id == 3)
    {
        // withdrawPosition <-------------->    assemblyPosition
        // 0                <-------------->    CANNULA_STROKE
        int16_t withdrawPosition = servos[servoID - 1].withdrawPosition;
        int16_t assemblyPosition = servos[servoID - 1].assemblyPosition;

        step = (withdrawPosition - assemblyPosition) / (0 - CANNULA_STROKE) * position + withdrawPosition;
    }
    else
    {
        PrintInfo("Sub-electrode ID Input ERROR!", '*');
        return;
    }
#ifdef ELECTRODE_DEBUG
    PrintSeperatingLine('-');
    Serial.printf("Electrode id: %d, Servo id: %d\n", id, servoID);
    Serial.print("Cannula destiniation position: ");
    Serial.print(position);
    Serial.printf(" mm, Servo target step: %d\n", step);
    PrintSeperatingLine('-');
#endif
    // 给对应servo发送位移指令
    LobotSerialServoMove(CONTROL_SERIAL, servoID, step, time);
}

/* 导丝移动函数 */
void StyletMove(uint8_t id, double position, uint16_t time)
{
    position = constrain(position, 0.0, 52.0);
    // 获得该导丝对应的舵机的 id
    uint8_t servoID = sub_electrodes[id - 1].styletServoID;
    int step;

    if (id == 1 || id == 2 || id == 3)
    {
        // withdrawPosition <-------------->    assemblyPosition
        // 0                <-------------->    CANNULA_STROKE
        int16_t withdrawPosition = servos[servoID - 1].withdrawPosition;
        int16_t assemblyPosition = servos[servoID - 1].assemblyPosition;

        step = (withdrawPosition - assemblyPosition) / (0 - STYLET_STROKE) * position + withdrawPosition;
    }
    else
    {
        Serial.println("Sub-electrode ID Input ERROR!");
        return;
    }
#ifdef ELECTRODE_DEBUG
    PrintSeperatingLine('-');
    Serial.printf("Electrode id: %d, Servo id: %d\n", id, servoID);
    Serial.print("Stylet destiniation position: ");
    Serial.print(position);
    Serial.printf(" mm, Servo target step: %d\n", step);
    PrintSeperatingLine('-');
#endif
    // 给对应servo发送位移指令
    LobotSerialServoMove(CONTROL_SERIAL, servoID, step, time);
}

/* 获取指定 servo id 的 servo 的步数 */
int GetServoStep(uint8_t id, uint8_t retryTimes)
{
#ifdef ELECTRODE_DEBUG
    PrintSeperatingLine('-');
#endif
    int ret;
    while (retryTimes > 0)
    {
        ret = LobotSerialServoReadPosition(CONTROL_SERIAL, id);
        if (ret != -2048 && ret != -2049)
        {

#ifdef ELECTRODE_DEBUG
            Serial.printf("Successfully obatin servo (id: %d) step: %d\n", id, ret);
#endif
            break;
        }
        retryTimes--;
#ifdef ELECTRODE_DEBUG
        Serial.printf("Get current step of servo (id: %d, return value: %d) error! Retry times left: %d\n",
                      id, ret, retryTimes);
#endif
    }
#ifdef ELECTRODE_DEBUG
    PrintSeperatingLine('-');
#endif
    return ret;
}

void SelfExamination()
{
    PrintInfo("Self Examination start", '*');
    PrintSeperatingLine('-');
    int steps[6];
    for (size_t i = 1; i <= 6; i++)
    {
        int step = GetServoStep(i, 5);
        if (step != -2048 && step != -2049)
        {
            steps[i - 1] = step;
        }
    }

    PrintSeperatingLine('-');
    // 显示electrode的配置信息
    for (size_t i = 0; i < 3; i++)
    {
        sub_electrode se = sub_electrodes[i];
        Serial.printf("[sub_electrode]\n");
        Serial.printf("\t id: %d\n", se.id);

        Serial.printf("\t [cannula servo]\n");
        Serial.printf("\t\t id: %d\n", se.cannulaServoID);
        Serial.printf("\t\t assemlby position: %d\n", servos[se.cannulaServoID - 1].assemblyPosition);
        Serial.printf("\t\t withdraw position: %d\n", servos[se.cannulaServoID - 1].withdrawPosition);
        Serial.printf("\t\t current step: %d\n", steps[se.cannulaServoID - 1]);

        Serial.printf("\t [stylet servo]\n");
        Serial.printf("\t\t id: %d\n", se.styletServoID);
        Serial.printf("\t\t assemlby position: %d\n", servos[se.styletServoID - 1].assemblyPosition);
        Serial.printf("\t\t withdraw position: %d\n", servos[se.styletServoID - 1].withdrawPosition);
        Serial.printf("\t\t current step: %d\n", steps[se.styletServoID - 1]);
    }
    PrintSeperatingLine('-');
    PrintInfo("Self Examination end", '*');
}

/* 获得给定子电极 ID 的外套管位置，单位：mm */
double GetCannulaPosition(uint8_t id)
{

    // 获得该cannula对应的servo ID
    uint8_t servoID = sub_electrodes[id - 1].cannulaServoID;

    // 获得该servo的位置,若获取出错（无响应或者处理出错，则重试）
    int ret = GetServoStep(servoID, 5);

    if (ret != -2048 && ret != -2049)
    {
        double step = ret * 1.0;

        // withdrawPosition    <===>    assemblyPosition     <===>     step
        // 0                   <===>    CANNULA_STROKE       <===>     answer

        // 获得该servo的装配状态位置
        int16_t withdrawPosition = servos[servoID - 1].withdrawPosition;
        // 获得该servo的收回状态位置
        int16_t assemblyPosition = servos[servoID - 1].assemblyPosition;

        // 根据servo的位置计算cannula对应的位置
        double position = (step - withdrawPosition) * (0 - CANNULA_STROKE) /
                          (withdrawPosition - assemblyPosition);

#ifdef ELECTRODE_DEBUG
        // Serial.printf 没有办法格式化浮点数
        PrintSeperatingLine('-');
        Serial.printf("Sub-electrode id: : %d, Current Cannula position: ", id);
        Serial.print(position);
        Serial.println(" mm");
        PrintSeperatingLine('-');
#endif
        return constrain(position, 0, 20);
    }
    else
    {
        // 可能出现获取到的位置有问题的情况
        return -1;
    }
}

double GetStyletPosition(uint8_t id)
{
    // 获得该stylet对应的servo ID
    uint8_t servoID = sub_electrodes[id - 1].styletServoID;

    // 获得该servo的位置,若获取出错（无响应或者处理出错，则重试）
    int ret = GetServoStep(servoID, 5);

    if (ret != -2048 && ret != -2049)
    {
        double step = ret;

        // withdrawPosition    <===>    assemblyPosition     <===>     step
        // 0                   <===>    STYLET_STROKE        <===>     answer

        // 获得该servo的装配状态位置
        int16_t withdrawPosition = servos[servoID - 1].withdrawPosition;
        // 获得该servo的收回状态位置
        int16_t assemblyPosition = servos[servoID - 1].assemblyPosition;
        // 根据servo的位置计算stylet对应的位置
        double position = (step - withdrawPosition) * (0 - STYLET_STROKE) / (withdrawPosition - assemblyPosition);

#ifdef ELECTRODE_DEBUG
        PrintSeperatingLine('-');
        Serial.printf("Sub-electrode id: : %d, Current Stylet position: ", id);
        Serial.print(position);
        Serial.println(" mm");
        PrintSeperatingLine('-');
#endif
        return constrain(position, 0, 52);
    }
    else
    {
        // 可能出现获取到的位置有问题的情况
        return -1;
    }
}

/* 将导丝收回外套管的函数 */
bool styletWithdraw(uint16_t time)
{
    PrintInfo("Stylet withdraw begin", '*');
    double cannulaPositions[3];
    for (size_t i = 0; i < 3; i++)
    {
        // 获得外套管位置
        double cannulaPosition = GetCannulaPosition(sub_electrodes[i].id);
        if (cannulaPosition == -1)
        {
            // 任何一个外套管位置获取错误均会导致此方法不会做任何动作
            PrintInfo("Cannula Position Fetch Error!", '*');
            return false;
        }
        else
        {
            // 保存获取到的外套管位置
            cannulaPositions[i] = cannulaPosition;
        }
    }
    for (size_t i = 0; i < 3; i++)
    {
        // 将导丝收回至外套管中
        StyletMove(sub_electrodes[i].id, cannulaPositions[i], time);
        delay(1);
    }
    delay(time);
    PrintInfo("All Stylet Withdrawed!", '*');
    return true;
}

/*
电极收回函数

电极收回包含两部分运动：
1. 导丝收回外套管中
2. 导丝和外套管一起收回至刚性穿刺外鞘中

注意：运行此函数以堵塞方式运行，直到运动到收回状态
*/
bool ElectrodePositionWithdraw(uint16_t time)
{
    PrintSeperatingLine('*');
    PrintInfo("Electrode Withdraw Starts!", '*');

    unsigned long start_time = millis();

    // 将 Stylet 收回外套管
    if (styletWithdraw(time))
    {
        // 同时收回外套管和导丝
        for (size_t i = 0; i < 3; i++)
        {
            CannulaMove(sub_electrodes[i].id, 0, time);
            delay(1);
            StyletMove(sub_electrodes[i].id, 0, time);
            delay(1);
        }
        // 等待 Cannula 和 Stylet 整体收回穿刺外鞘的运动完成
        delay(time);

        unsigned long end_time = millis();

        PrintInfo("All Cannula Withdrawed!", '*');
        PrintInfo("Electrode Withdraw ends!", '*');
        Serial.printf("Presumed Time Cost: %d ms, Practical Time Cost: %d ms\n", time << 1, end_time - start_time);
        PrintSeperatingLine('*');

        return true;
    }
    else
    {
        PrintInfo("Electrode Withdraw failed", '*');
        return false;
    }
}

/*
电极展开函数

电极展开包含三部分运动：
1. 导丝收回外套管中
2. 导丝和外套管一起穿刺进入组织,到达外套管目标位置
3. 导丝展开

注意：运行此函数将堵塞线程，直到运动到目标状态。
*/
bool ElectrodePositionExpand(double electrodePosition[6], uint16_t time)
{
    PrintSeperatingLine('*');
    PrintInfo("Electrode Expand Starts!", '*');
    unsigned long start_time = millis();

    for (size_t i = 0; i < 3; i++)
    {
        // 导丝和外套管一起穿刺进入组织,到达外套管目标位置
        CannulaMove(sub_electrodes[i].id, electrodePosition[2 * i], time);
        delay(1);
        StyletMove(sub_electrodes[i].id, electrodePosition[2 * i], time);
        delay(1);
    }
    delay(time);

    PrintInfo("Cannula Expanded!", '*');

    for (size_t i = 0; i < 3; i++)
    {
        // 导丝展开
        StyletMove(sub_electrodes[i].id, electrodePosition[2 * i + 1], time);
    }
    delay(time);

    unsigned long end_time = millis();
    PrintInfo("Stylet Expanded!", '*');
    PrintInfo("Electrode Expand ends!", '*');
    Serial.printf("Presumed Time Cost: %d ms, Practical Time Cost: %d ms\n", time * 3, end_time - start_time);
    PrintSeperatingLine('*');
    return true;
}

/* 返回装配位置函数 */
void BackToAssemblyPosition()
{
    size_t back_to_assembly_state_time = 3000;

    PrintSeperatingLine('*');
    PrintInfo("Backing to the assembly position", '*');

    // 默认在 4 秒内完成所有 servo 的回装配位置的运动
    for (size_t i = 0; i < 6; i++)
    {
        LobotSerialServoMove(CONTROL_SERIAL, servos[i].id, servos[i].assemblyPosition,
                             back_to_assembly_state_time);
        delay(1);
    }
    delay(back_to_assembly_state_time);
    PrintInfo("READY FOR ASSEMBLY!", '*');
    delay(1000);
}

/* 装配位置初始化函数 */
void ElectrodePositionAssembly()
{
    BackToAssemblyPosition();

    // 等待串口上有传输过来的数据，此时任何数据通过串口发送均表示电极已安装完毕，可以进行展开等工作
    while (Serial.available() <= 0)
    {
        ;
    }
    if (Serial.available() > 0)
    {
        // 接受串口数据，但是并不进行任何操作
        while (Serial.available() > 0)
        {
            Serial.read();
            delay(5); // 等待一会再接收下一个
        }
    }
    delay(1000);
    PrintInfo("ASSEMBLY DONE!", '*');
    PrintSeperatingLine('*');
    delay(1000);
}

void AblationElectrodeInit()
{
    // 设置串口0的波特率，用于和上位机通信
    Serial.begin(9600);

    // 设置控制信号串口的波特率，用于和 buslinker 通信，间接控制舵机
    // 115200 bps 代表 1 second 可以传输 115200 bit，因此 1 bit 传输时间换算出来就是 0.0658 ms
    CONTROL_SERIAL.begin(115200);

    delay(1000);

    ServoDefinition();
    ElectrodeDefinition();

    SelfExamination();

    ElectrodePositionAssembly();
}