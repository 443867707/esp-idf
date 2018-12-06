#include "Includes.h"
#include "cli-interpreter.h"
#include "APP_LampCtrl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void CommandGetWs(void);
void CommandResetWs(void);
void CommandGetStatusWs(void);
void CommandGetWsFlash(void);


CommandEntry CommandTablWs[] =
{
	CommandEntryActionWithDetails("dump", CommandGetWs, "", "work scheme dump...", (void*)0),
	CommandEntryActionWithDetails("dumpFlash", CommandGetWsFlash, "", "work scheme dump flash...", (void*)0),
	CommandEntryActionWithDetails("reset", CommandResetWs, "", "work scheme reset...", (void*)0),
	CommandEntryActionWithDetails("status", CommandGetStatusWs, "", "get work scheme status...", (void*)0),

	CommandEntryTerminator()
};


void WorkSchCliDump(ST_WORK_SCHEME * pstPlanScheme)
{
	int i = 0;
	char week[8] ={0};

	if(NULL == pstPlanScheme){
		CLI_PRINTF("OppLampCtrlWorkSchDump pstPlanScheme is NULL\r\n");
		return;
	}
	CLI_PRINTF("\r\n~~~~~~~~~~OppLampCtrlWorkSchDump plan %d~~~~~~~~~~~~~\r\n", pstPlanScheme->hdr.index);
	CLI_PRINTF("used %d\r\n", pstPlanScheme->used);
	CLI_PRINTF("index %d priority %d valide %d mode %d\r\n", pstPlanScheme->hdr.index, pstPlanScheme->hdr.priority, pstPlanScheme->hdr.valid, pstPlanScheme->hdr.mode);
	if(WEEK_MODE == pstPlanScheme->hdr.mode){
		if(pstPlanScheme->u.stWeekPlan.dateValide){
			CLI_PRINTF("sDate: y %04d m %02d d %02d h %02d m %02d s %02d\r\n", 
				pstPlanScheme->u.stWeekPlan.sDate.Year,
				pstPlanScheme->u.stWeekPlan.sDate.Month,
				pstPlanScheme->u.stWeekPlan.sDate.Day,
				pstPlanScheme->u.stWeekPlan.sDate.Hour,
				pstPlanScheme->u.stWeekPlan.sDate.Sencond,
				pstPlanScheme->u.stWeekPlan.sDate.Min);
			CLI_PRINTF("eDate: y %04d m %02d d %02d h %02d m %02d s %02d\r\n", 
				pstPlanScheme->u.stWeekPlan.eDate.Year,
				pstPlanScheme->u.stWeekPlan.eDate.Month,
				pstPlanScheme->u.stWeekPlan.eDate.Day,
				pstPlanScheme->u.stWeekPlan.eDate.Hour,
				pstPlanScheme->u.stWeekPlan.eDate.Sencond,
				pstPlanScheme->u.stWeekPlan.eDate.Min);
			for(i=0;i<7;i++){
				if(pstPlanScheme->u.stWeekPlan.bitmapWeek & 1<<i){
					//cnt++;
					week[i] = '1';
				}
				else{
					week[i] = '0';
				}
			}
			week[i] = '\0';
			CLI_PRINTF("weekbitmap: %s\r\n", week);
		}
	}
	else if(YMD_MODE == pstPlanScheme->hdr.mode
		||AST_MODE == pstPlanScheme->hdr.mode){
		CLI_PRINTF("sDate: y %04d m %02d d %02d h %02d m %02d s %02d\r\n", 
			pstPlanScheme->u.stYmdPlan.sDate.Year,
			pstPlanScheme->u.stYmdPlan.sDate.Month,
			pstPlanScheme->u.stYmdPlan.sDate.Day,
			pstPlanScheme->u.stYmdPlan.sDate.Hour,
			pstPlanScheme->u.stYmdPlan.sDate.Sencond,
			pstPlanScheme->u.stYmdPlan.sDate.Min);
		CLI_PRINTF("eDate: y %04d m %02d d %02d h %02d m %02d s %02d\r\n", 
			pstPlanScheme->u.stYmdPlan.eDate.Year,
			pstPlanScheme->u.stYmdPlan.eDate.Month,
			pstPlanScheme->u.stYmdPlan.eDate.Day,
			pstPlanScheme->u.stYmdPlan.eDate.Hour,
			pstPlanScheme->u.stYmdPlan.eDate.Sencond,
			pstPlanScheme->u.stYmdPlan.eDate.Min);
	}
	
	CLI_PRINTF("jobNum %d\r\n", pstPlanScheme->jobNum);

	for(i=0;i<JOB_MAX;i++){
		if(1 != pstPlanScheme->jobs[i].used)
			continue;
		
		if(AST_MODE == pstPlanScheme->hdr.mode){
			CLI_PRINTF("used %d interv %d dim %d\r\n", pstPlanScheme->jobs[i].used, pstPlanScheme->jobs[i].intervTime,
				pstPlanScheme->jobs[i].dimLevel);
		}else{
			CLI_PRINTF("used %d start %02d:%02d, %02d:%02d dim %d\r\n", pstPlanScheme->jobs[i].used,
				pstPlanScheme->jobs[i].startHour,pstPlanScheme->jobs[i].startMin,
				pstPlanScheme->jobs[i].endHour,pstPlanScheme->jobs[i].endMin,
				pstPlanScheme->jobs[i].dimLevel);
		}
	
	}
	CLI_PRINTF("\r\n~~~~~~~~~~~~~~~~~~~~~~~\r\n");
}

void CommandGetWs(void)
{
	int i;
	
	for(i=0;i<PLAN_MAX;i++){
		CLI_PRINTF("id=%d\r\n", i);
		WorkSchCliDump(&g_stPlanScheme[i]);
	}
}

void CommandGetWsFlash(void)
{
	int i;
	OppLampCtrlReadWorkSchemeFromFlash();
	for(i=0;i<PLAN_MAX;i++){
		CLI_PRINTF("id=%d\r\n", i);
		WorkSchCliDump(&g_stPlanScheme[i]);
	}
}

void CommandResetWs(void)
{
	int i;
	for(i=0;i<PLAN_MAX;i++){
		OppLampCtrlDelWorkScheme(i);		
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
void CommandGetStatusWs(void)
{
	CLI_PRINTF("phase:%s\r\n",	OppLampCtrlGetPhase()==0?"find dimmer policy":"do dimmer policy");
	if(1 == OppLampCtrlGetPhase()){
		CLI_PRINTF("planId:%d\r\n", OppLampCtrlGetPlanIdx());
		CLI_PRINTF("jobsId:%d\r\n", OppLampCtrlGetJobsIdx());
	}
}
