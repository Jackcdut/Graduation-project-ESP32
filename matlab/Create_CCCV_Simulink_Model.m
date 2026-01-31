%% Create CC/CV Power Supply Simulink Model - Correct Implementation
% This script creates a properly working Simulink model for CC/CV control
% Key fix: Use proper feedback structure and tuned PID parameters
% Author: Power Electronics Lab
% Date: 2024

clear; clc; close all;

%% ==================== Model Parameters ====================
V_set = 12;         % Voltage setpoint (V)
I_set = 2;          % Current limit setpoint (A)
V_in = 24;          % Input voltage (V)

% PID Parameters - Voltage Loop (tuned for stability)
Kp_v = 1;
Ki_v = 500;
Kd_v = 0;

% PID Parameters - Current Loop (faster response for protection)
Kp_i = 2;
Ki_i = 1000;
Kd_i = 0;

% Power Stage Time Constant
tau_ps = 0.001;     % 1ms

% Load profile
load_time = [0; 0.015; 0.03; 0.045; 0.06; 0.08; 0.1];
load_values = [24; 12; 6; 4; 3; 10; 10];
R_load_data = timeseries(load_values, load_time);

% Save to workspace
assignin('base', 'V_set', V_set);
assignin('base', 'I_set', I_set);
assignin('base', 'V_in', V_in);
assignin('base', 'Kp_v', Kp_v);
assignin('base', 'Ki_v', Ki_v);
assignin('base', 'Kd_v', Kd_v);
assignin('base', 'Kp_i', Kp_i);
assignin('base', 'Ki_i', Ki_i);
assignin('base', 'Kd_i', Kd_i);
assignin('base', 'tau_ps', tau_ps);
assignin('base', 'R_load_data', R_load_data);

%% ==================== Create Simulink Model ====================
modelName = 'CCCV_Power_Supply';

if bdIsLoaded(modelName)
    close_system(modelName, 0);
end
if exist([modelName '.slx'], 'file')
    delete([modelName '.slx']);
end

new_system(modelName);
open_system(modelName);

set_param(modelName, 'StopTime', '0.1');
set_param(modelName, 'Solver', 'ode23tb');
set_param(modelName, 'MaxStep', '1e-4');
set_param(modelName, 'RelTol', '1e-4');

%% ==================== Add Subsystem: Power Stage ====================
% Create a subsystem for cleaner organization
add_block('simulink/Ports & Subsystems/Subsystem', [modelName '/PowerStage']);
delete_line([modelName '/PowerStage'], 'In1/1', 'Out1/1');
delete_block([modelName '/PowerStage/In1']);
delete_block([modelName '/PowerStage/Out1']);

% Add ports to subsystem
add_block('simulink/Sources/In1', [modelName '/PowerStage/Duty']);
set_param([modelName '/PowerStage/Duty'], 'Position', [50, 100, 80, 114]);

add_block('simulink/Sources/In1', [modelName '/PowerStage/R_load']);
set_param([modelName '/PowerStage/R_load'], 'Position', [50, 200, 80, 214], 'Port', '2');

add_block('simulink/Sinks/Out1', [modelName '/PowerStage/V_out']);
set_param([modelName '/PowerStage/V_out'], 'Position', [400, 100, 430, 114]);

add_block('simulink/Sinks/Out1', [modelName '/PowerStage/I_out']);
set_param([modelName '/PowerStage/I_out'], 'Position', [400, 200, 430, 214], 'Port', '2');

% Gain (Vin)
add_block('simulink/Math Operations/Gain', [modelName '/PowerStage/Vin']);
set_param([modelName '/PowerStage/Vin'], 'Gain', 'V_in', 'Position', [120, 95, 150, 125]);

% Transfer function (power stage dynamics)
add_block('simulink/Continuous/Transfer Fcn', [modelName '/PowerStage/TF']);
set_param([modelName '/PowerStage/TF'], 'Numerator', '[1]', 'Denominator', '[tau_ps 1]', ...
    'Position', [180, 92, 260, 128]);

% Saturation for voltage (0 to Vin)
add_block('simulink/Discontinuities/Saturation', [modelName '/PowerStage/Vsat']);
set_param([modelName '/PowerStage/Vsat'], 'UpperLimit', 'V_in', 'LowerLimit', '0', ...
    'Position', [290, 95, 320, 125]);

% Divide for current calculation
add_block('simulink/Math Operations/Divide', [modelName '/PowerStage/Idiv']);
set_param([modelName '/PowerStage/Idiv'], 'Position', [290, 190, 320, 230]);

% Connect inside subsystem
add_line([modelName '/PowerStage'], 'Duty/1', 'Vin/1');
add_line([modelName '/PowerStage'], 'Vin/1', 'TF/1');
add_line([modelName '/PowerStage'], 'TF/1', 'Vsat/1');
add_line([modelName '/PowerStage'], 'Vsat/1', 'V_out/1');
add_line([modelName '/PowerStage'], 'Vsat/1', 'Idiv/1', 'autorouting', 'on');
add_line([modelName '/PowerStage'], 'R_load/1', 'Idiv/2');
add_line([modelName '/PowerStage'], 'Idiv/1', 'I_out/1');

set_param([modelName '/PowerStage'], 'Position', [400, 150, 500, 230]);

%% ==================== Main Model Blocks ====================

% === Setpoints ===
add_block('simulink/Sources/Constant', [modelName '/V_set']);
set_param([modelName '/V_set'], 'Value', 'V_set', 'Position', [30, 55, 60, 85]);

add_block('simulink/Sources/Constant', [modelName '/I_set']);
set_param([modelName '/I_set'], 'Value', 'I_set', 'Position', [30, 255, 60, 285]);

% === Voltage Loop ===
add_block('simulink/Math Operations/Sum', [modelName '/V_err']);
set_param([modelName '/V_err'], 'Inputs', '+-', 'Position', [120, 60, 140, 80]);

add_block('simulink/Continuous/PID Controller', [modelName '/PID_V']);
set_param([modelName '/PID_V'], ...
    'Controller', 'PI', ...
    'P', 'Kp_v', 'I', 'Ki_v', ...
    'LimitOutput', 'on', ...
    'UpperSaturationLimit', '1', 'LowerSaturationLimit', '0', ...
    'AntiWindupMode', 'clamping', ...
    'Position', [180, 52, 240, 88]);

% === Current Loop ===
add_block('simulink/Math Operations/Sum', [modelName '/I_err']);
set_param([modelName '/I_err'], 'Inputs', '+-', 'Position', [120, 260, 140, 280]);

add_block('simulink/Continuous/PID Controller', [modelName '/PID_I']);
set_param([modelName '/PID_I'], ...
    'Controller', 'PI', ...
    'P', 'Kp_i', 'I', 'Ki_i', ...
    'LimitOutput', 'on', ...
    'UpperSaturationLimit', '1', 'LowerSaturationLimit', '0', ...
    'AntiWindupMode', 'clamping', ...
    'Position', [180, 252, 240, 288]);

% === OR-Gate (Min for low-select) ===
add_block('simulink/Math Operations/MinMax', [modelName '/MIN']);
set_param([modelName '/MIN'], 'Function', 'min', 'Inputs', '2', ...
    'Position', [300, 145, 330, 185]);

% === Load ===
add_block('simulink/Sources/From Workspace', [modelName '/R_load']);
set_param([modelName '/R_load'], 'VariableName', 'R_load_data', ...
    'Position', [300, 215, 360, 245]);

% === Feedback routing ===
add_block('simulink/Signal Routing/Goto', [modelName '/Vout_tag']);
set_param([modelName '/Vout_tag'], 'GotoTag', 'Vout', 'Position', [560, 157, 610, 183]);

add_block('simulink/Signal Routing/Goto', [modelName '/Iout_tag']);
set_param([modelName '/Iout_tag'], 'GotoTag', 'Iout', 'Position', [560, 197, 610, 223]);

add_block('simulink/Signal Routing/From', [modelName '/Vfb']);
set_param([modelName '/Vfb'], 'GotoTag', 'Vout', 'Position', [70, 97, 110, 123]);

add_block('simulink/Signal Routing/From', [modelName '/Ifb']);
set_param([modelName '/Ifb'], 'GotoTag', 'Iout', 'Position', [70, 297, 110, 323]);

% === Scopes ===
add_block('simulink/Sinks/Scope', [modelName '/Scope_V']);
set_param([modelName '/Scope_V'], 'NumInputPorts', '2', 'Position', [700, 50, 730, 80]);

add_block('simulink/Sinks/Scope', [modelName '/Scope_I']);
set_param([modelName '/Scope_I'], 'NumInputPorts', '2', 'Position', [700, 250, 730, 280]);

add_block('simulink/Sinks/Scope', [modelName '/Scope_Ctrl']);
set_param([modelName '/Scope_Ctrl'], 'NumInputPorts', '3', 'Position', [400, 45, 430, 95]);

% === From blocks for scopes ===
add_block('simulink/Signal Routing/From', [modelName '/Vout_scope']);
set_param([modelName '/Vout_scope'], 'GotoTag', 'Vout', 'Position', [640, 62, 680, 88]);

add_block('simulink/Signal Routing/From', [modelName '/Iout_scope']);
set_param([modelName '/Iout_scope'], 'GotoTag', 'Iout', 'Position', [640, 262, 680, 288]);

% === Mux for control scope ===
add_block('simulink/Signal Routing/Mux', [modelName '/Ctrl_mux']);
set_param([modelName '/Ctrl_mux'], 'Inputs', '3', 'Position', [350, 47, 355, 93]);

%% ==================== Connect Main Model ====================

% Voltage loop
add_line(modelName, 'V_set/1', 'V_err/1');
add_line(modelName, 'Vfb/1', 'V_err/2');
add_line(modelName, 'V_err/1', 'PID_V/1');
add_line(modelName, 'PID_V/1', 'MIN/1', 'autorouting', 'on');

% Current loop
add_line(modelName, 'I_set/1', 'I_err/1');
add_line(modelName, 'Ifb/1', 'I_err/2');
add_line(modelName, 'I_err/1', 'PID_I/1');
add_line(modelName, 'PID_I/1', 'MIN/2', 'autorouting', 'on');

% MIN to Power Stage
add_line(modelName, 'MIN/1', 'PowerStage/1');
add_line(modelName, 'R_load/1', 'PowerStage/2');

% Power Stage outputs
add_line(modelName, 'PowerStage/1', 'Vout_tag/1');
add_line(modelName, 'PowerStage/2', 'Iout_tag/1');

% Voltage scope
add_line(modelName, 'V_set/1', 'Scope_V/1', 'autorouting', 'on');
add_line(modelName, 'Vout_scope/1', 'Scope_V/2');

% Current scope
add_line(modelName, 'I_set/1', 'Scope_I/1', 'autorouting', 'on');
add_line(modelName, 'Iout_scope/1', 'Scope_I/2');

% Control scope
add_line(modelName, 'PID_V/1', 'Ctrl_mux/1', 'autorouting', 'on');
add_line(modelName, 'PID_I/1', 'Ctrl_mux/2', 'autorouting', 'on');
add_line(modelName, 'MIN/1', 'Ctrl_mux/3', 'autorouting', 'on');
add_line(modelName, 'Ctrl_mux/1', 'Scope_Ctrl/1');

%% ==================== Save and Run ====================
save_system(modelName);

fprintf('\n');
fprintf('============================================================\n');
fprintf('   Simulink Model: %s.slx\n', modelName);
fprintf('============================================================\n');
fprintf('\n');
fprintf('Parameters:\n');
fprintf('  V_set = %d V, I_set = %d A, V_in = %d V\n', V_set, I_set, V_in);
fprintf('  R_critical = V_set/I_set = %d Ohm\n', V_set/I_set);
fprintf('\n');
fprintf('Load Profile:\n');
fprintf('  0-15ms:  R=24 -> CV, I=0.5A\n');
fprintf('  15-30ms: R=12 -> CV, I=1.0A\n');
fprintf('  30-45ms: R=6  -> CV, I=2.0A (boundary)\n');
fprintf('  45-60ms: R=4  -> CC, V=8V, I=2A\n');
fprintf('  60-80ms: R=3  -> CC, V=6V, I=2A\n');
fprintf('  80-100ms:R=10 -> CV, I=1.2A\n');
fprintf('\n');
fprintf('Scopes:\n');
fprintf('  Scope_V:    Yellow=V_set(12V), Blue=V_out\n');
fprintf('  Scope_I:    Yellow=I_set(2A),  Blue=I_out\n');
fprintf('  Scope_Ctrl: Yellow=PID_V, Blue=PID_I, Red=MIN output\n');
fprintf('\n');
fprintf('Running simulation...\n');

% Run simulation
simOut = sim(modelName, 'StopTime', '0.1');

fprintf('Simulation complete!\n');
fprintf('============================================================\n');
