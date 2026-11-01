# Calculator controls / 计算器操作

The default is **Keypad**. The game is designed around the calculator's numeric
keys, Enter, Esc and Menu. Press **0** to explicitly switch between **Keypad** and
**Touch aim**. Moving or resting a finger on the touchpad never switches modes.
The current mode is shown in the interface and can also be changed in Settings.

默认使用**按键模式**，以计算器数字区、Enter、Esc、Menu 为主要操作。
按 **0** 明确切换**按键 / 触摸瞄准**；轻触或移动手指不会擅自切换模式。
界面显示当前模式，设置中也可切换。

| Action / 动作 | Calculator keys / 计算器按键 |
| --- | --- |
| Thrust / 推进 | **8** or the direction pad's upper edge / 方向盘上沿 |
| Turn / 左右转向 | **4 / 6** or the direction pad's left / right edge / 方向盘左右沿 |
| Brake / 刹车 | **2** or the direction pad's lower edge / 方向盘下沿 |
| Fire / 开火 | **5** or **Ctrl** |
| Ability / manual construct deployment / 技能与手动部署已组装构造体 | **7** or **Tab** |
| Pause / resume / 暂停与继续 | **Esc**, **Menu** or **Doc** |
| Move through menus / 菜单移动 | **8 / 2** or the direction pad / 方向盘 |
| Confirm / 确认 | **5 / Enter** or press the direction pad center / 方向盘中心 |
| Back / 返回 | **Esc** |
| Choose upgrade / 选择升级 | **8 / 2 / 4 / 6**, then **5 / Enter** / 移动后确认；数字方向键不会购买 |
| Reroll upgrades / 重掷升级 | **−** (minus / 减号) |
| Banish a mod, when available / 已解锁的模组放逐 | **Del** |
| Bank / spend upgrade points / 保留或使用升级点 | **9**; **Esc** also banks points on the upgrade screen / 升级页也可按 Esc 保留 |
| Switch input mode / 切换输入模式 | **0** |

**Keypad mode.** Press the touchpad rim to use it as a direction pad; touching it
without pressing does nothing. A press belongs to the sector where it started.
Dragging that held press across the center will not accidentally confirm a
menu item. Numeric keys remain available, including 5 to fire while steering.
If a numeric direction and a held touchpad edge disagree, the numeric key
controls that axis until released; the other touchpad axis remains available.
Moving through a menu while pressing the pad center never also confirms it;
release and press the center again to confirm.
In menus, an overridden pad direction also waits for a new press, so releasing
a numeric key cannot produce a delayed move in the opposite direction.

**按键模式。** 按下触摸板边缘才触发方向，轻触无效。同一次按压固定使用
开始时的方向区，按住后滑过中心不会误确认。数字区仍可同时操作，例如
转向时按 5 开火。
如果数字方向键与仍按住的触摸板边缘方向冲突，以数字键控制该轴，松开后
恢复触摸板方向；另一个轴仍可同时使用。菜单移动时同时按住中央不会误确认，
需要松开中央并重新按下才确认。
菜单中被数字键覆盖的触摸板方向同样需要重新按下，松开数字键不会触发
迟到的反向移动。

**Touch aim mode.** Point to aim, press the touchpad to fire, and use 8 to thrust
or 2 to brake. In menus, point and make a fresh click to choose an item; hovering
does not change keyboard selection. Pressing the rim in this mode is a pointer
click, never an extra direction key. Numeric 4 / 6 temporarily takes precedence
while held, without changing the selected input mode. Press 0 to return to
Keypad. When changing modes while a finger is resting on the pad, lift it once
before using it in the new mode. Held numeric controls also wait for release;
release and press again after switching.

**触摸瞄准模式。** 指向瞄准，按下触摸板开火，8 推进、2 刹车。在菜单里
指向后重新按下才选择；悬停不会覆盖键盘选中项。边缘点击不会同时变成
方向键。按住 4 / 6 时暂以按键转向，松开仍保持触摸模式。按 0 返回按键模式；
切换时若手指仍放在触摸板上，请先抬起一次；同时按住的数字键也需松开再按。

**Page changes.** A key or pointer already held when a page opens is ignored
until released. Release 5/Enter after confirming, then press again for the next
page. The click used to leave a menu cannot also fire, select an upgrade, or
activate the new page.

**页面切换。** 新页面会等待原来按住的键或触摸板释放；确认后先松开 5/Enter，
下页再按。离开菜单的旧点击不会穿透到游戏、升级或下一页。

**Rebind controls.** Open Settings → Controls → Define keyboard controls. Select
one of the nine actions, press Enter, release it, then press the desired physical
calculator key. A used key is rejected until you choose a free key; Esc cancels.
Enter, Esc, Menu, Doc, Home and 0 stay reserved. Menu confirmation stays on 5
even if Fire is rebound. Ability/upgrade/reroll/banish cannot take 5 or the
numeric navigation keys, so navigation and mode switching remain accessible. Changes are saved immediately to `state/nNova.keys.tns`,
with a checksum and a recoverable previous copy. Reset at the bottom of the list
restores the numeric layout. Rebinding a flight action replaces its default
letter and modifier aliases; fixed menu navigation remains 8 / 2 / 4 / 6.

**重新绑定。** 打开“设置 → 控制 → 自定义键盘控制”，选择九个动作之一，
按 Enter 并松开，再按想使用的计算器实体键。已被占用的按键会被拒绝，
可以继续选择空闲键；Esc 取消。Enter、Esc、Menu、Doc、Home、0 保留给
菜单与模式切换；改绑开火后，5 仍固定用于菜单确认，技能、升级、重掷、放逐
不能占用 5 或数字方向键。更改会立即保存到 `state/nNova.keys.tns`，附带校验与旧版备份。
列表底部的重置恢复数字键布局。自定义飞行动作会替换其原有字母和修饰键
别名；菜单仍固定使用 8 / 2 / 4 / 6。

The ship has momentum: thrust accelerates in the direction it faces and release
lets it drift. Salvo builds its missile stockpile while not firing. Vortex
charges while firing and releases when firing stops. Optional automatic fire
is available in Settings: each fresh fire press toggles firing on or off, so
you can deliberately stop firing to charge Salvo or release Vortex. Auto Save defaults to **Paused Only**, saving when pausing or upgrading. Select
**Always** to additionally save every 10 seconds, or **Never** to use only
explicit saves. The Pause menu's **Save and Quit** always saves before returning
to the title. A death invalidates the saved run in every mode.

Settings use **4 / 6** for tabs, **8 / 2** for rows, **5 / Enter** to change a value,
and **7** for the selected setting's complete description. Each construct type
can auto-deploy independently; when its setting is off, **7** deploys that type
once it finishes assembling. Existing constructs remain active.

飞船具有惯性，推进沿朝向加速，松开后会漂移。Salvo 在未开火时积蓄导弹，
Vortex 按住开火蓄力、松开发射。设置内可启用自动开火，每次重新按下
开火键切换开火或停火，以便控制 Salvo 积蓄与 Vortex 释放。自动保存默认“仅暂停时”，暂停和升级时保存；
选“总是”还会每十秒保存，选“从不”则只使用明确的保存操作。
暂停菜单的“保存并退出”始终保存并返回标题；死亡始终使本局存档失效。
设置页用 **4 / 6** 切换标签、**8 / 2** 选项目、**5 / Enter** 更改、**7** 查看完整说明。
无人机、地雷、炮塔、盟友可分别关闭自动部署；关闭后按 **7** 部署已经组装完成的
对应构造体，已部署单位继续工作。

Desktop preview: arrows and the numeric keypad follow the same layout. The
mouse is enabled by Touch aim mode (0). W/A/S/D, Space/Z, X/Shift, P, R and U
remain optional desktop aliases for movement, fire, thrust, pause, reroll and
upgrades; they are not required on the calculator.

**Offline daily challenge.** Select Daily Challenge on the challenge-mode page.
Use **8 / 2** to choose Year, Month, Day, Start, Rules or Local Scores; **4 / 6**
changes the selected date field, **5 / Enter** confirms, and **7** opens the
complete challenge and modifier rules. A configured calculator clock supplies
the date. If its clock was reset, select the date or reuse the last saved date.
Every date has a repeatable local challenge. Retrying keeps the same date and
seed. The best score for each of 16 dates is stored locally, separately from
ordinary runs; choosing a score selects that date for replay. These challenges
use an offline rotation and are not the online PC leaderboard.

**离线每日挑战。** 在挑战模式页选择每日挑战。**8 / 2** 选择年、月、日、开始、
规则或本地成绩，**4 / 6** 修改选中的日期字段，**5 / Enter** 确认，**7** 查看
完整挑战与修饰规则。计算器时钟已设置时自动取日期；时钟被重置后可选择日期，
或沿用上次保存的日期。每个日期对应可重复的本地挑战；重试保留相同日期和种子。
最近16个日期各自的最佳成绩单独保存，选择成绩可重玩对应日期。采用离线轮换，
不接入电脑版在线排行榜。
