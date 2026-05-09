package com.minefilter.agent;

import java.lang.instrument.Instrumentation;

public class AgentMain {
    public static void premain(String agentArgs, Instrumentation inst) {
        System.out.println("[MineFilter Agent] Loading...");
        inst.addTransformer(new RendererTransformer());
    }
}
