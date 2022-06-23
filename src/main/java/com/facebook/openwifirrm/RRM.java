/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

package com.facebook.openwifirrm;

import com.facebook.openwifirrm.modules.ApiServer;
import com.facebook.openwifirrm.modules.ConfigManager;
import com.facebook.openwifirrm.modules.DataCollector;
import com.facebook.openwifirrm.modules.Modeler;
import com.facebook.openwifirrm.mysql.DatabaseManager;
import com.facebook.openwifirrm.ucentral.KafkaConsumerRunner;
import com.facebook.openwifirrm.ucentral.UCentralClient;
import com.facebook.openwifirrm.ucentral.UCentralKafkaConsumer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.stream.Collectors;

/**
 * RRM service runner.
 */
public class RRM {
	private static final Logger logger = LoggerFactory.getLogger(RRM.class);

	/** The executor service instance. */
	private final ExecutorService executor = Executors.newCachedThreadPool();

	/**
	 * Wrap a {@code Runnable} as a {@code Callable<Object>}.
	 *
	 * This is similar to {@link Executors#callable(Runnable)} but will log any
	 * exceptions thrown and then call {@link System#exit(int)}.
	 */
	private static Callable<Object> wrapRunnable(Runnable task) {
		return () -> {
			try {
				task.run();
			} catch (Exception e) {
				logger.error("Exception raised in task!", e);
				System.exit(1);
			}
			return null;
		};
	}

	/** Start the RRM service. */
	public boolean start(
		RRMConfig config,
		DeviceDataManager deviceDataManager,
		UCentralClient client,
		UCentralKafkaConsumer consumer,
		DatabaseManager dbManager
	) {
		// Instantiate modules
		ConfigManager configManager = new ConfigManager(
			config.moduleConfig.configManagerParams, deviceDataManager, client
		);
		DataCollector dataCollector = new DataCollector(
			config.moduleConfig.dataCollectorParams,
			deviceDataManager,
			client,
			consumer,
			configManager,
			dbManager
		);
		Modeler modeler = new Modeler(
			config.moduleConfig.modelerParams,
			deviceDataManager,
			consumer,
			client,
			dataCollector,
			configManager
		);
		ApiServer apiServer = new ApiServer(
			config.moduleConfig.apiServerParams,
			deviceDataManager,
			configManager,
			modeler
		);
		KafkaConsumerRunner consumerRunner =
			(consumer == null) ? null : new KafkaConsumerRunner(consumer);

		// Add shutdown hook
		Runtime.getRuntime().addShutdownHook(new Thread(() -> {
			logger.debug("Running shutdown hook...");
			if (consumerRunner != null) {
				consumerRunner.shutdown();
			}
			apiServer.shutdown();
			dataCollector.shutdown();
			executor.shutdownNow();
		}));

		List<Runnable> runnables = new ArrayList<>(List.of(
			configManager, dataCollector, modeler, apiServer, consumerRunner
		));

		if (config.provConfig.useVenuesEnabled) {
			ProvMonitor provMonitor = new ProvMonitor(
				configManager,
				deviceDataManager,
				modeler,
				client
			);
			runnables.add(provMonitor);
		}

		// Submit jobs
		List<Callable<Object>> services = runnables.stream()
			.filter(o -> o != null)
			.map(RRM::wrapRunnable)
			.collect(Collectors.toList());
		try {
			executor.invokeAll(services);
		} catch (InterruptedException e) {
			logger.info("Execution interrupted!", e);
			return true;
		}

		// All jobs crashed?
		return false;
	}
}
