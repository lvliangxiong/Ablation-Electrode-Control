// 用于是否开启调试
#define ELECTRODE_DEBUG 1;

#define SERVO_ID1 1
#define SERVO_ID2 2
#define SERVO_ID3 3
#define SERVO_ID4 4
#define SERVO_ID5 5
#define SERVO_ID6 6

#define CONTROL_SERIAL Serial1

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

void PrintElectrodeInfo(sub_electrode se)
{
    Serial.println("--------------------------------------");
    Serial.print("sub_electrode id: ");
    Serial.println(se.id);

    Serial.print("cannula servo id: ");
    Serial.print(se.cannulaServoID);
    Serial.print("  ");
    Serial.print("stylet servo id: ");
    Serial.println(se.styletServoID);

    Serial.println("--------------------------------------");
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
        if (position[cannula] < 0.0 || position[cannula] > 20.0)
        {
            Serial.println("Cannula out of the range, please check!");
            return false;
        }
        // 导丝的位移需要在 [0, 52] mm 之间
        if (position[stylet] < 0.0 || position[stylet] > 52.0)
        {
            Serial.println("Stylet out of the range, please check!");
            return false;
        }
        // 限制stylet的位置，最多只能刚好完全收回,当然也不能超过cannula的限位
        if (position[stylet] < position[cannula] ||
            position[stylet] > position[cannula] + 32.0)
        {
            Serial.println("Stylet out of the range, please check!");
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
        Serial.println("Sub-electrode ID Input ERROR!");
        return;
    }
#ifdef ELECTRODE_DEBUG
    Serial.println("--------------------------------------");
    Serial.print("Electrode id: ");
    Serial.print(id);
    Serial.print("  ");
    Serial.print("Servo id: ");
    Serial.println(servoID);
    Serial.print("Cannula destiniation position: ");
    Serial.print(position);
    Serial.print(" mm");
    Serial.print("  ");
    Serial.print("Servo target step: ");
    Serial.println(step);
    Serial.println("--------------------------------------");
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
    Serial.println("--------------------------------------");
    Serial.print("Electrode id: ");
    Serial.print(id);
    Serial.print("  ");
    Serial.print("Servo id: ");
    Serial.println(servoID);
    Serial.print("Stylet destiniation Position: ");
    Serial.print(position);
    Serial.print(" mm");
    Serial.print("  ");
    Serial.print("Servo target step: ");
    Serial.println(step);
    Serial.println("--------------------------------------");
#endif
    // 给对应servo发送位移指令
    LobotSerialServoMove(CONTROL_SERIAL, servoID, step, time);
}

/* 获得给定子电极 ID 的外套管位置，单位：mm */
double GetCannulaPosition(uint8_t id)
{
    // 获得该cannula对应的servo ID
    uint8_t servoID = sub_electrodes[id - 1].cannulaServoID;

    // 获得该servo的位置,若获取出错（无响应或者处理出错，则重试）
    uint8_t count = 5;
    int ret;
    while (count > 0)
    {
        ret = LobotSerialServoReadPosition(CONTROL_SERIAL, servoID);
        if (ret != -2048 && ret != -2049)
        {
            break;
        }
        count--;
#ifdef ELECTRODE_DEBUG
        Serial.println("--------------------------------------");
        Serial.print("Get Cannula Position ERROR!   ");
        Serial.print("TRIED TIMES: ");
        Serial.println(5 - count);
        Serial.println("--------------------------------------");
#endif
    }
    double step = ret;

    // 获得该servo的装配状态位置
    int16_t withdrawPosition = servos[servoID - 1].withdrawPosition;
    // 获得该servo的收回状态位置
    int16_t assemblyPosition = servos[servoID - 1].assemblyPosition;
    // 根据servo的位置计算cannula对应的位置
    double position = (step - withdrawPosition) * (0 - CANNULA_STROKE) / (withdrawPosition - assemblyPosition);

#ifdef ELECTRODE_DEBUG
    Serial.println("--------------------------------------");
    Serial.print("Current Position: ");
    Serial.print(position);
    Serial.print(" mm");
    Serial.print("  ");
    Serial.print("step ");
    Serial.println(step);
    Serial.println("--------------------------------------");
#endif

    // 可能出现获取到的位置有问题的情况，需要仔细考虑获取到的位置有误时应该如何处理
    return constrain(position, 0, 20);
}

double GetStyletPosition(uint8_t cannulaID)
{
    uint8_t servoID = sub_electrodes[cannulaID - 1].styletServoID;
    double step = LobotSerialServoReadPosition(CONTROL_SERIAL, servoID);
    int16_t withdrawPosition = servos[servoID - 1].withdrawPosition;
    int16_t assemblyPosition = servos[servoID - 1].assemblyPosition;
    return (step - withdrawPosition) * (0 - STYLET_STROKE) / (withdrawPosition - assemblyPosition);
}

/* 
电极收回函数

电极收回包含两部分运动：
1. 导丝收回外套管中
2. 导丝和外套管一起收回至刚性穿刺外鞘中

注意：运行此函数以堵塞方式运行，直到运动到收回状态
*/
void ElectrodePositionWithdraw(uint16_t time)
{
    Serial.println("");
    Serial.println("#####################################################");
    Serial.println("*********** Electrode Withdraw Starts! ***********");
    unsigned long start_time = millis();
    for (size_t i = 0; i < 3; i++)
    {
        Serial.print("Sub-electrode id: ");
        Serial.println(i + 1);

        // 获得外套管位置
        double cannulaPosition = GetCannulaPosition(sub_electrodes[i].id);
        // 将导丝收回至外套管中
        StyletMove(sub_electrodes[i].id, cannulaPosition, time);
    }

    // 等待 Stylet 收回外套管的运动完成
    delay(time);

    Serial.println("*********** All Stylet Withdrawed! ***********");

    for (size_t i = 0; i < 3; i++)
    {
        Serial.print("Sub-electrode id: ");
        Serial.println(i + 1);
        // 同时收回外套管和导丝
        CannulaMove(sub_electrodes[i].id, 0, time);
        StyletMove(sub_electrodes[i].id, 0, time);
    }
    // 等待 Cannula 和 Stylet 整体收回穿刺外鞘的运动完成
    delay(time);

    unsigned long end_time = millis();
    Serial.println("*********** All Cannula Withdrawed! ***********");
    Serial.println("*********** Electrode Withdraw ends! ***********");
    Serial.print("Presumed Time Cost: ");
    Serial.print(time << 1);
    Serial.println(" ms");
    Serial.print("Practical Time Cost: ");
    Serial.print(end_time - start_time);
    Serial.println(" ms");
    Serial.println("#####################################################");
}

/* 
电极展开函数

电极展开包含三部分运动：
1. 导丝收回外套管中
2. 导丝和外套管一起穿刺进入组织,到达外套管目标位置
3. 导丝展开

注意：运行此函数将堵塞线程，直到运动到目标状态。
*/
void ElectrodePositionExpand(double electrodePosition[6], uint16_t time)
{
    Serial.println("");
    Serial.println("#####################################################");
    Serial.println("*********** Electrode Expand Starts! ***********");
    unsigned long start_time = millis();
    for (size_t i = 0; i < 3; i++)
    {
        // 获得外套管位置
        double cannulaPosition = GetCannulaPosition(sub_electrodes[i].id);
        // 将导丝收回至外套管中
        StyletMove(sub_electrodes[i].id, cannulaPosition, time);
    }
    delay(time);
    Serial.println("*********** Stylet Withdrawed! ***********");

    for (size_t i = 0; i < 3; i++)
    {
        // 导丝和外套管一起穿刺进入组织,到达外套管目标位置
        CannulaMove(sub_electrodes[i].id, electrodePosition[2 * i], time);
        StyletMove(sub_electrodes[i].id, electrodePosition[2 * i], time);
    }
    delay(time);
    Serial.println("*********** Cannula Expanded! ***********");

    for (size_t i = 0; i < 3; i++)
    {
        // 导丝展开
        StyletMove(sub_electrodes[i].id, electrodePosition[2 * i + 1], time);
    }
    delay(time);
    unsigned long end_time = millis();
    Serial.println("*********** Stylet Expanded! ***********");
    Serial.println("*********** Electrode Expand ends! ***********");
    Serial.print("Presumed Time Cost: ");
    Serial.print(time * 3);
    Serial.println(" ms");
    Serial.print("Practical Time Cost: ");
    Serial.print(end_time - start_time);
    Serial.println(" ms");
    Serial.println("#####################################################");
}

/* 装配位置初始化函数 */
void ElectrodePositionAssembly()
{
    Serial.println("");
    Serial.println("#####################################################");
    Serial.println("*********** Backing to the assembly state! ***********");

    // 默认在 4 秒内完成所有 servo 的回装配位置的运动
    for (size_t i = 0; i < 6; i++)
    {
        LobotSerialServoMove(CONTROL_SERIAL, servos[i].id, servos[i].assemblyPosition, 4000);
    }
    delay(4000);
    Serial.println("*********** READY FOR ASSEMBLY! ***********");

    // 等待串口上有传输过来的数据，此时任何数据通过串口发送均表示电极已安装完毕，可以进行展开等工作
    while (Serial.available() <= 0)
    {
        ;
    }
    if (Serial.available() > 0)
    {
        // 接受串口数据，并不进行任何操作
        while (Serial.available() > 0)
        {
            Serial.read();
        }
    }
    delay(1000);
    Serial.println("*********** ASSEMBLY DONE! ***********");
    Serial.println("#####################################################");
    delay(1000);
}

/* 返回装配位置函数 */
void BackToAssemblyPosition()
{
    Serial.println("");
    Serial.println("#####################################################");
    Serial.println("*********** Backing to the assembly position! ***********");

    // 默认在 4 秒内完成所有 servo 的回装配位置的运动
    for (size_t i = 0; i < 6; i++)
    {
        LobotSerialServoMove(CONTROL_SERIAL, servos[i].id, servos[i].assemblyPosition, 4000);
    }
    delay(4000);
    Serial.println("*********** DONE！ ***********");
    Serial.println("#####################################################");
    delay(1000);
}

void AblationElectrodeInit()
{
    // 设置串口0的波特率
    Serial.begin(115200);

    // 设置控制信号串口的波特率
    CONTROL_SERIAL.begin(115200);

    delay(1000);

    ServoDefinition();
    ElectrodeDefinition();
    ElectrodePositionAssembly();

    // 显示electrode的配置信息
    for (size_t i = 0; i < 3; i++)
    {
        PrintElectrodeInfo(sub_electrodes[i]);
    }
}